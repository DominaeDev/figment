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
	model_params.use_mlock = false;
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
	ctx_params.pooling_type = LLAMA_POOLING_TYPE_LAST;

	_pCtx = llama_init_from_model(_pModel, ctx_params);
	if (!_pCtx)
	{
		fprintf(stderr, "%s: error: failed to create the llama_context\n", __func__);
		return false;
	}

//	llama_token cls = llama_model_decoder_start_token(_pModel);
//	const char* tmpl = llama_model_chat_template(_pModel, nullptr);

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

static std::vector<llama_token> TokenizeSentences(llama_model* pModel, llama_context* pCtx, Sentences sentences)
{
	const int32_t ctx_size = llama_n_ctx(pCtx);
	const llama_vocab* pVocab = llama_model_get_vocab(pModel);

	std::vector<llama_token> tokens;
	tokens.reserve(ctx_size);

	int n = 0;
	for (auto it = sentences.crbegin(); it != sentences.crend(); ++it, ++n)
	{
		auto msg_tokens = llm_util::tokenize(pModel, (*it).sentence, false);
		if (msg_tokens.size() > ctx_size - tokens.size() - 2) // Account for <bos> <eos>
			break;
		tokens.insert(tokens.begin(), msg_tokens.cbegin(), msg_tokens.cend());
		if (n > 0)
			tokens.insert(tokens.begin(), llama_vocab_sep(pVocab));	// <SEP>
	}

	llama_token tok_bos = llama_vocab_bos(pVocab); // <BOS>
	llama_token tok_eos = llama_vocab_bos(pVocab); // <EOS>
	if (tok_bos > 0)
		tokens.insert(tokens.begin(), tok_bos);
	if (tok_eos > 0)
		tokens.insert(tokens.end(), tok_eos);
	return tokens;
}

bool LLMEmbedding::Search(const Sentences& sentences, bool bUser, bool bBot)
{
	if (sentences.empty())
		return false;

	Sentences searchSentences = FilterContainer(sentences, [bUser, bBot](const Sentence& s) { 
		return (bBot && (is_bot(s.role) || s.role == Role::Narrator)) || (bUser && s.role == Role::User); 
	});

	if (!searchSentences.empty())
	{
		auto tokens = TokenizeSentences(_pModel, _pCtx, searchSentences);
		Embedding embedding;
		if (!__Generate(tokens, "", Mode::Query, embedding))
			return false;
#if _DEBUG
		CompareSimilarity(embedding.vec, sentences.size());
#endif
	}
	return true;
}

bool LLMEmbedding::Generate(std::string text, Embedding& out_embedding)
{
	string content = text;
	const llama_vocab* pVocab = llama_model_get_vocab(_pModel);
	std::vector<llama_token> tokens;

	llama_token tok_bok = llama_vocab_bos(pVocab);
	llama_token tok_eos = llama_vocab_eos(pVocab);
	llama_token tok_sep = llama_vocab_sep(pVocab);

#if EMBEDDING_SPLIT_SENTENCES
	auto sentences = string_util::split(text, { '.', ';' }, true);
	int n = 0;
	for (auto it = sentences.crbegin(); it != sentences.crend(); ++it)
	{
		auto msg_tokens = llm_util::tokenize(_pModel, *it, false);
		tokens.insert(tokens.begin(), msg_tokens.cbegin(), msg_tokens.cend());
		if (n > 0 && tok_sep > 0)
			tokens.insert(tokens.begin(), tok_sep);
		if (++n == 1)
			break;
	}
#else
	tokens = llm_util::tokenize(_pModel, text, false);
#endif

	if (tok_bok > 0)
		tokens.insert(tokens.begin(), tok_bok);
	if (tok_eos > 0)
		tokens.insert(tokens.end(), tok_eos);

	return __Generate(tokens, content, Mode::Document, out_embedding);
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


bool LLMEmbedding::__Generate(const std::vector<llama_token>& in_tokens, string content, Mode mode, Embedding& out_embedding)
{
	int32_t ctx_size = llama_n_ctx(_pCtx);

	std::vector<llama_token> tokens = in_tokens;
	std::vector<llama_token> instructions;
#if NOMIC_EMBEDDING
	if (mode == Mode::Query)
		instructions = llm_util::tokenize(_pModel, "search_query: ", false);
	else if (mode == Mode::Document)
		instructions = llm_util::tokenize(_pModel, "search_document: ", false);
#else
//	if (mode == Mode::Query)
//		instructions = llm_util::tokenize(_pModel, "Represent the statement: ", false);
//	else if (mode == Mode::Document)
//		instructions = llm_util::tokenize(_pModel, "Represent the statement: ", false);
#endif
	tokens.insert(tokens.begin(), instructions.begin(), instructions.end());

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
void LLMEmbedding::CompareSimilarity(const std::vector<float>& vec, size_t n_sentences)
{
	auto& embeddings = Embeddings::GetEmbeddings();

	std::vector<std::pair<float, string>> results;
	const size_t MaxLength = 64;
	for (size_t i = 0; i < embeddings.size(); ++i)
	{
		if (embeddings[i].vec.size() != vec.size())
			continue;

		float similarity = common_embd_similarity_cos(embeddings[i].vec.data(), vec.data(), (int32_t)vec.size());
		similarity = powf(similarity, 1.5f);

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