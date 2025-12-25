#include <pch.h>
#include "llm/LLMEngine.h"
#include "llm/LLMInstance.h"
#include "llm/LLMTemplate.h"
#include "llm/LLMEmbedding.h"
#include "llm/LLMStatus.h"

#include "util/Lockable.h"
#include "util/StringUtility.h"
#include "util/Common.h"
#include "util/FileUtility.h"

using namespace fig::common_util;
using namespace fig::string_util;
using namespace fig::file_util;
using namespace fig::llm;

using __LlamaLogCallback = std::function<void(ggml_log_level level, const char* text, void* user_data)>;
static void OnLlamaLog(ggml_log_level level, const char* text, void* user_data)
{
	LLMEngine* pThis = static_cast<LLMEngine*>(user_data);

	fig::string log(text);
	size_t pos_eq = log.find('=');
	bool bGPU = log.find("CUDA") != fig::npos;
	bool bCPU = log.find("CPU") != fig::npos;

	if (pos_eq != fig::npos && log.find("buffer size") != fig::npos)
	{
		fig::string value = trim(log.substr(pos_eq + 1));
		double mul = 1024.0 * 1024.0; // MiB
		if (ends_with(value, "GiB"))
			mul *= 1024.0;
		try
		{
			int64_t iValue = toI64(std::stod(value) * mul);
			if (bGPU)
				pThis->usedVRAM.fetch_add(iValue);
			else if (bCPU)
				pThis->usedRAM.fetch_add(iValue);
		}
		catch (...)
		{
			// Do nothing
		}
	}

	DebugPrint(text);
}

bool LLMEngine::OnLoadModelProgress(float progress, void* user_data)
{
	LLMEngine* pThis = static_cast<LLMEngine*>(user_data);
	if (pThis->_pLoadModelProgressCallback)
		pThis->_pLoadModelProgressCallback(static_cast<int>(progress * 100.0f));
	return true;
}

LLMEngine::LLMEngine()
{
	_pStatus = std::make_shared<LLMStatusChannel>();
}

bool LLMEngine::Initialize(fig::string modelFilename, fig::string embeddingFilename, LoadModelProgressCallback onProgress, LoadModelCallback onComplete)
{
	auto readyState = _readyState.load();
	if (readyState > ReadyState::Uninitialized)
		return false; // Already loading or loaded

	SetReadyState(ReadyState::Initializing);
	_pLoadModelProgressCallback = onProgress;

	_pStatus->EmitSignal(LLMStatusSignal::ModelLoading);

	_workerThread = std::make_unique<std::jthread>(std::jthread(std::bind_front(&LLMEngine::__LoadModel, this), 
		modelFilename,
		embeddingFilename,
		[this, modelFilename, embeddingFilename, onComplete](std::shared_ptr<ModelState> result)
		{
			if (result->pModel && result->pCtx) // Success
			{
				LockAndDo([this, &result]() {
					_modelState = result;
				}, _stateMutex);

				SetReadyState(ReadyState::Ready);
				onComplete(true);

				_pStatus->ReportMemory(usedRAM.load(), usedVRAM.load(), false);
				_pStatus->EmitSignal(LLMStatusSignal::ModelLoaded);
				DebugPrintLn("Loaded model OK");
			}
			else // Failure
			{
				result->Release();
				LockAndDo([this]() {
					_modelState.reset();
				}, _stateMutex);

				SetReadyState(ReadyState::LoadError);
				onComplete(false);

				_pStatus->ReportMemory(0, 0, false);
				_pStatus->EmitSignal(LLMStatusSignal::ModelLoadFailure);
				DebugPrintLn("Failed to load model");
			}
		}));

	return true;
}

bool LLMEngine::Shutdown()
{
	// Stop all instances
	for (auto& instance : _instances)
		instance->Shutdown();
	_instances.clear(); // Free instances

	LockAndDo([this]() {
		// Clear and release state
		if (_modelState)
		{
			_modelState->Release();
			_modelState.reset();
		}

	}, _stateMutex);
	
	SetReadyState(ReadyState::Uninitialized);
	_pStatus->EmitSignal(LLMStatusSignal::ModelUnloaded);

	llama_backend_free();
	return true;
}

void LLMEngine::SetReadyState(ReadyState readyState)
{
	_readyState.store(readyState);
	_pStatus->ReportReadyState(readyState);
}

void LLMEngine::__LoadModel(fig::string modelFilename, fig::string embeddingFilename, __LoadModelCallback onComplete)
{
	const int ngl = 99; // All layers
	usedVRAM.store(0);
	usedRAM.store(0);

	llama_backend_init();

	llama_log_set(OnLlamaLog, (void*)this);

	// initialize the model
	llama_model_params model_params = llama_model_default_params();
	model_params.n_gpu_layers = ngl;
	model_params.use_mmap = true;
	model_params.use_mlock = true;
	model_params.progress_callback = (llama_progress_callback)&OnLoadModelProgress;
	model_params.progress_callback_user_data = (void*)this;

	auto state = std::make_shared<ModelState>();
	state->pModel = llama_model_load_from_file(modelFilename.c_str(), model_params);
	if (!state->pModel)
	{
		fprintf(stderr, "%s: error: unable to load model\n", __func__);
		onComplete(state);
		return;
	}

	state->pVocab = llama_model_get_vocab(state->pModel);
	state->modelName = GetFilename(modelFilename);
	llm_tmpl::auto_detect_template(state->pModel);

	// initialize the context
	llama_context_params ctx_params = llama_context_default_params();
	int32_t n_ctx = std::min(llama_model_n_ctx_train(state->pModel), Constants::Context::DefaultSize);
	int32_t n_seq_max = Constants::Context::MaxSequences;
	ctx_params.n_ctx = n_ctx;
	ctx_params.n_seq_max = n_seq_max;
	ctx_params.n_batch = n_ctx;
	ctx_params.n_ubatch = Constants::Context::MicroBatchSize;

	state->pCtx = llama_init_from_model(state->pModel, ctx_params);
	state->ctx_size = llama_n_ctx(state->pCtx);
	state->num_sequences = n_seq_max;

	if (!state->pCtx)
	{
		fprintf(stderr, "%s: error: failed to create the llama_context\n", __func__);
		onComplete(state);
		return;
	}

	// Initialize embedder
	if (!embeddingFilename.empty())
	{
		state->pEmbedding = new LLMEmbedding {};
		if (state->pEmbedding->LoadModel(embeddingFilename))
			DebugPrintLn("Loaded embedding model");
		else
		{
			delete state->pEmbedding;
			state->pEmbedding = nullptr;
		}
	}

	onComplete(state);
}

LLMInstancePtr LLMEngine::CreateInstance(int32_t ctx_size, bool embeddings)
{
	auto pInstance = std::make_shared<LLMInstance>();
	pInstance->_modelState = *_modelState.get();
	pInstance->_pStatus = _pStatus;
	_instances.push_back(pInstance);
	return pInstance;
}

bool LLMEngine::DestroyInstance(LLMInstancePtr instance)
{
	auto itFind = std::ranges::find(_instances, instance);
	if (itFind != std::ranges::end(_instances))
	{
		itFind->get()->Shutdown();
		_instances.erase(itFind);
		return true;
	}
	return false;
}