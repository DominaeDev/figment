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

bool LLMEmbedding::Generate(const std::vector<string>& history, std::vector<float>& out_embedding)
{
	if (history.empty())
		return false;

	int32_t ctx_size = llama_n_ctx(_pCtx);
	const llama_vocab* pVocab = llama_model_get_vocab(_pModel);

	std::vector<llama_token> tokens;
	tokens.reserve(ctx_size);

	int n = 0;
	for (auto it = history.crbegin(); it != history.crend(); ++it)
	{
		auto msg_tokens = llm_util::tokenize(_pModel, *it, false);
		if (msg_tokens.size() > ctx_size - tokens.size())
			break;
		tokens.insert(tokens.begin(), msg_tokens.cbegin(), msg_tokens.cend());
		tokens.insert(tokens.begin(), llama_vocab_sep(pVocab));
		
		if (++n == 4)
			break;
	}
	tokens[0] = llama_vocab_bos(pVocab);

	if (!__Generate(tokens, out_embedding))
		return false;

#if _DEBUG
	for (size_t i = 0; i < n_test_embeddings; ++i)
	{
		float similarity = common_embd_similarity_cos(test_embeddings[i], out_embedding.data(), (int32_t)n_embed);
		DebugPrintLn(std::format("Similarity [{0}] = {1}", (int32_t)i, similarity));
	}
#endif
	return true;
}

bool LLMEmbedding::Generate(std::string text, std::vector<float>& out_embedding)
{
	std::vector<llama_token> tokens = llm_util::tokenize(_pModel, text, false);
	const llama_vocab* pVocab = llama_model_get_vocab(_pModel);
	tokens.insert(tokens.begin(), llama_vocab_bos(pVocab));
	return __Generate(tokens, out_embedding);
}

bool LLMEmbedding::__Generate(const std::vector<llama_token>& tokens, std::vector<float>& out_embedding)
{
	int32_t ctx_size = llama_n_ctx(_pCtx);

	llama_batch batch;
	if (!llm_util::init_embedding_batch(_pModel, _pCtx, tokens, batch))
		return false;

	if (batch.n_tokens > ctx_size)
		batch.n_tokens = ctx_size;
	batch.logits[batch.n_tokens - 1] = true;

	llama_kv_self_clear(_pCtx);
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
	return true;
}