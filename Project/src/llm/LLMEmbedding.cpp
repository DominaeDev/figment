#include "llm/LLMEmbedding.h"
#include "llm/LLMUtility.h"
#include "util/StringUtility.h"
#include "util/Common.h"
#include <llama.h>
#include <common.h>
#include <format>

#include "llm/TestEmbeddings.h" //! @temp

LLMEmbedding::~LLMEmbedding()
{
	Shutdown();
}

bool LLMEmbedding::LoadModel(string filename)
{
	const int ngl = 99; // All layers
	const int n_ctx = 512;

	// initialize the model
	llama_model_params model_params = llama_model_default_params();
	model_params.n_gpu_layers = ngl;
	model_params.use_mlock = true;

	_pModel = llama_model_load_from_file(filename.c_str(), model_params);
	
	if (!_pModel)
	{
		fprintf(stderr, "%s: error: unable to load model\n", __func__);
		return false;
	}

	const int ctx_size = llama_model_n_ctx_train(_pModel);

	// initialize the context
	llama_context_params ctx_params = llama_context_default_params();
	ctx_params.n_ctx = ctx_size;
	ctx_params.n_batch = ctx_size;
	ctx_params.embeddings = true;
	ctx_params.pooling_type = LLAMA_POOLING_TYPE_MEAN;

	_pCtx = llama_init_from_model(_pModel, ctx_params);
	if (!_pCtx)
	{
		fprintf(stderr, "%s: error: failed to create the llama_context\n", __func__);
		return false;
	}

	n_embed = llama_model_n_embd(_pModel);

	return true;
}

void LLMEmbedding::Shutdown()
{
	if (_pCtx)
	{
		llama_kv_self_clear(_pCtx);
		llama_free(_pCtx);
	}

	if (_pModel)
		llama_model_free(_pModel);

	if (_pBatch)
		llama_batch_free(*_pBatch);

	_pModel = nullptr;
	_pCtx = nullptr;
	_pBatch = nullptr;
}

bool LLMEmbedding::IsReady() const
{
	return _pModel != nullptr && _pCtx != nullptr;
}

bool LLMEmbedding::Generate(string text, std::vector<float>& out_embedding)
{
	auto tokens = llm_util::tokenize(_pModel, text, false);

	llama_batch batch;
	if (!llm_util::init_embedding_batch(_pModel, _pCtx, text, batch))
		return false;

	if (llama_decode(_pCtx, batch) != 0)
	{
		llama_batch_free(batch);
		return false;
	}
	const float * embedding = llama_get_embeddings_seq(_pCtx, 0);
	if (embedding == nullptr)
	{
		llama_batch_free(batch);
		return false;
	}
	llama_batch_free(batch);

	out_embedding = std::vector<float>(embedding, embedding + n_embed);
	common_embd_normalize(out_embedding.data(), out_embedding.data(), (int32_t)out_embedding.size(), 2); // 2 = euclidean

#if _DEBUG
	string asString;
	asString.reserve(2048);
	asString.append(std::format("float embedding[{}] = {{\r\n", out_embedding.size()));
	int n = 0;
	for (float f : out_embedding)
	{
		asString.append(std::format("\t{}f, ", f));
		if (++n == 8)
		{
			n = 0;
			asString.append("\r\n");
		}
	}
	asString.append("};");

	for (size_t i = 0; i < n_test_embeddings; ++i)
	{
		float similarity = common_embd_similarity_cos(embedding, test_embeddings[i], (int32_t)n_embed);
		DebugPrintLn(std::format("Similarity [{0}] = {1}", (int32_t)i, similarity));
	}
#endif
	return true;
}