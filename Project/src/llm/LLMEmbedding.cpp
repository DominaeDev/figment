#include "llm/LLMEmbedding.h"
#include "llm/LLMUtility.h"
#include "util/StringUtility.h"
#include "util/Common.h"
#include "Constants.h"
#include <llama.h>
#include <common.h>
#include <format>
#include <cassert>

LLMEmbedding::~LLMEmbedding()
{
	Shutdown();
}

bool LLMEmbedding::LoadModel(string filename)
{
	const int ngl = 99; // All layers
	const int n_ctx = 384;

	// initialize the model
	llama_model_params model_params = llama_model_default_params();
	model_params.n_gpu_layers = ngl;
	model_params.use_mlock = true;

	_pModel = llama_model_load_from_file(filename.c_str(), model_params);
	_modelName = string_util::get_filename(filename);

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
//	ctx_params.n_seq_max = 1;
	ctx_params.pooling_type = LLAMA_POOLING_TYPE_MEAN;

	_pCtx = llama_init_from_model(_pModel, ctx_params);
	if (!_pCtx)
	{
		fprintf(stderr, "%s: error: failed to create the llama_context\n", __func__);
		return false;
	}

	n_embed = llama_model_n_embd(_pModel);

	Embeddings::Initialize("./embeddings/", _modelName);

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

bool LLMEmbedding::Generate(const std::vector<string>& history, Embedding& out_embedding)
{
	if (history.empty())
		return false;

	int32_t ctx_size = llama_n_ctx(_pCtx);
	const llama_vocab* pVocab = llama_model_get_vocab(_pModel);

	std::vector<llama_token> tokens;
	tokens.reserve(ctx_size);

#if NOMIC_EMBEDDING
	auto prefix_tokens = llm_util::tokenize(_pModel, "search_query: ", false);
	prefix_tokens.insert(prefix_tokens.begin(), llama_vocab_bos(pVocab));
#else
	std::vector<llama_token> prefix_tokens;
	prefix_tokens.insert(prefix_tokens.begin(), llama_vocab_bos(pVocab)); // <BOS>
#endif

	int n = 0;
	for (auto it = history.crbegin(); it != history.crend(); ++it)
	{
		auto msg_tokens = llm_util::tokenize(_pModel, *it, false);
		if (msg_tokens.size() > ctx_size - tokens.size() - prefix_tokens.size() - 1)
			break;
		tokens.insert(tokens.begin(), msg_tokens.cbegin(), msg_tokens.cend());
		if (n > 0)
			tokens.insert(tokens.begin(), llama_vocab_sep(pVocab));	
		if (++n == 1)
			break;
	}
	tokens.insert(tokens.begin(), prefix_tokens.begin(), prefix_tokens.end()); // Prefix
	tokens.insert(tokens.end(), llama_vocab_eos(pVocab)); // <EOS>

	if (!__Generate(tokens, "", out_embedding))
		return false;

#if _DEBUG
	CompareSimilarity(out_embedding.vec);
#endif
	return true;
}

bool LLMEmbedding::Generate(std::string text, bool bSearch, Embedding& out_embedding)
{
	string content = text;
#if NOMIC_EMBEDDING
	if (bSearch)
		text = "search_query: " + text;
	else
		text = "search_document: " + text;
#endif

	std::vector<llama_token> tokens = llm_util::tokenize(_pModel, text, false);
	const llama_vocab* pVocab = llama_model_get_vocab(_pModel);
	tokens.insert(tokens.begin(), llama_vocab_bos(pVocab));

	return __Generate(tokens, content, out_embedding);
}

static bool batch_decode(llama_context* ctx, llama_batch& batch, float* output, int n_seq, int n_embd, int embd_norm)
{
	const enum llama_pooling_type pooling_type = llama_pooling_type(ctx);

	// clear previous kv_cache values (irrelevant for embeddings)
	llama_kv_self_clear(ctx);

	// run model
	if (llama_decode(ctx, batch) < 0)
		return false;

	for (int i = 0; i < batch.n_tokens; i++)
	{
		if (!batch.logits[i])
			continue;

		const float* embd = nullptr;
		int embd_pos = 0;

		if (pooling_type == LLAMA_POOLING_TYPE_NONE)
		{
			// try to get token embeddings
			embd = llama_get_embeddings_ith(ctx, i);
			embd_pos = i;
			assert(embd != NULL && "failed to get token embeddings");
		}
		else
		{
			// try to get sequence embeddings - supported only when pooling_type is not NONE
			embd = llama_get_embeddings_seq(ctx, batch.seq_id[i][0]);
			embd_pos = batch.seq_id[i][0];
			assert(embd != NULL && "failed to get sequence embeddings");
		}

		float* out = output + embd_pos * n_embd;
		common_embd_normalize(embd, out, n_embd, embd_norm);
	}
	return true;
}


bool LLMEmbedding::__Generate(const std::vector<llama_token>& tokens, string content, Embedding& out_embedding)
{
	int32_t ctx_size = llama_n_ctx(_pCtx);

	llama_batch batch;
	if (!llm_util::init_embedding_batch(_pModel, _pCtx, tokens, batch))
		return false;

	if (batch.n_tokens > ctx_size)
		batch.n_tokens = ctx_size;
	batch.logits[batch.n_tokens - 1] = true;

	std::vector<float> embeddings(n_embed, 0);
	float * embedding = embeddings.data();
	batch_decode(_pCtx, batch, embedding, 1, (int32_t)n_embed, 2);
	llama_batch_free(batch);

	out_embedding = Embedding {
		/*modelName*/ _modelName,
		/*content*/ content,
		std::vector<float>(embedding, embedding + n_embed),
	};
	return true;
}

#if _DEBUG
void LLMEmbedding::CompareSimilarity(const std::vector<float>& vec)
{
	auto& embeddings = Embeddings::GetEmbeddings();

	std::vector<std::pair<float, string>> results;
	const size_t MaxLength = 64;
	for (size_t i = 0; i < embeddings.size(); ++i)
	{
		if (embeddings[i].vec.size() != vec.size())
			continue;

		float similarity = common_embd_similarity_cos(embeddings[i].vec.data(), vec.data(), (int32_t)vec.size());
		string content = embeddings[i].content;
		if (content.size() > MaxLength)
			content = content.substr(0, MaxLength) + "...";
		results.push_back(std::make_pair(similarity, content));
	}
	std::sort(results.begin(), results.end(), [](auto a, auto b) {
		return a.first > b.first;
	});

	for (int i = 0; i < results.size() && i < 5; ++i)
		DebugPrintLn(std::format("Similarity {0:.3f} = \"{1}\"", results[i].first, results[i].second));
}
#endif