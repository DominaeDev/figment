#include <pch.h>
#include "llm/LLMEmbedding.h"
#include "llm/LLMUtility.h"
#include "llm/LlamaApi.h"
#include "util/StringUtility.h"
#include "fs/FileUtility.h"
#include "util/Common.h"
#include "Constants.h"
#include <llama.h>
#include <format>
#include <cassert>

using namespace fig::io;
using namespace fig::llm;
using namespace fig::util;

LLMEmbedding::~LLMEmbedding()
{
	Shutdown();
}

bool LLMEmbedding::LoadModel(fig::string filename)
{
	const int ngl = 99; // All layers
	const int n_ctx = Constants::Embedding::ContextSize;

	// initialize the model
	llama_model_params model_params = llama_model_default_params();
	model_params.n_gpu_layers = ngl;
	model_params.use_mmap = false;
	model_params.use_mlock = false;
	_pModel = llama_model_load_from_file(filename.c_str(), model_params);
	_modelName = GetFilename(filename);

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

//	llama_token cls = llama_model_decoder_start_token(_pModel);
//	const char* tmpl = llama_model_chat_template(_pModel, nullptr);

	n_embed = llama_model_n_embd(_pModel);

	Embeddings::Initialize(fig::string(Constants::Embedding::EmbeddingSaveLocation), _modelName);

	return true;
}

void LLMEmbedding::Shutdown()
{
	if (_pCtx)
	{
		llama::ctx_clear(_pCtx);
		llama::free(_pCtx);
		_pCtx = nullptr;
	}

	if (_pModel)
	{
		llama::free(_pModel);
		_pModel = nullptr;
	}
}

bool LLMEmbedding::IsReady() const
{
	return _pModel != nullptr && _pCtx != nullptr;
}

static std::vector<llama_token> TokenizeSentences(VocabPtr pVocab, ContextPtr pCtx, Sentences sentences)
{
	const int32_t ctx_size = llama::ctx_size(pCtx);

	std::vector<llama_token> tokens;
	tokens.reserve(ctx_size);

	int n = 0;
	for (auto it = sentences.crbegin(); it != sentences.crend(); ++it, ++n)
	{
		auto msg_tokens = llama::tokenize(pVocab, (*it).sentence, false);
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

	Sentences searchSentences = sentences
		| std::views::filter([bUser, bBot](const Sentence& s) {
			return (bBot && (is_bot(s.role) || s.role == Role::Narrator)) || (bUser && s.role == Role::User);
		})
		| std::ranges::to<Sentences>();

	if (!searchSentences.empty())
	{
		const llama_vocab* pVocab = llama::get_vocab(_pModel);
		auto tokens = TokenizeSentences(pVocab, _pCtx, searchSentences);
		EmbeddingVector embedding;
		if (!__Generate(tokens, "", Mode::Query, embedding))
			return false;

		if constexpr (Debugging)
			CompareSimilarity(embedding.vec, sentences.size());
	}
	return true;
}

bool LLMEmbedding::Generate(fig::string text, EmbeddingVector& out_embedding)
{
	fig::string content = text;
	const llama_vocab* pVocab = llama::get_vocab(_pModel);

	Sentences sentences;
	if (Constants::Embedding::SplitSentences)
	{
		auto strings = split(text, { '.', ';' }, true);
		for (auto& s : strings)
			sentences.push_back(Sentence { Role::Undefined, s});
	}
	else
	{
		sentences.push_back(Sentence { Role::Undefined, trim(text) });
	}

	auto tokens = TokenizeSentences(pVocab, _pCtx, sentences);

	return __Generate(tokens, content, Mode::Document, out_embedding);
}

static bool batch_decode(ContextPtr ctx, Batch& batch, float* output, int n_seq, int n_embd, int embd_norm)
{
	const enum llama_pooling_type pooling_type = llama_pooling_type(ctx);

	// clear previous kv_cache values (irrelevant for embeddings)
	llama::ctx_clear(ctx);

	// run model
	if (llama_decode(ctx, batch) < 0)
		return false;

	for (int i = 0; i < batch.n_tokens; i++)
	{
		if (batch.logits == nullptr || !batch.logits[i])
			continue;

		float* embd = nullptr;
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
		fig::llm::util::embd_normalize(embd, out, n_embd, embd_norm);
	}
	return true;
}


bool LLMEmbedding::__Generate(const std::vector<llama_token>& in_tokens, fig::string content, Mode mode, EmbeddingVector& out_embedding)
{
	int32_t ctx_size = llama::ctx_size(_pCtx);
	const llama_vocab* pVocab = llama::get_vocab(_pModel);

	std::vector<llama_token> tokens = in_tokens;
	std::vector<llama_token> instructions;
	if (mode == Mode::Query && !Constants::Embedding::QueryPrefix.empty())
		instructions = llama::tokenize(pVocab, toStr(Constants::Embedding::QueryPrefix), false);
	else if (mode == Mode::Document && !Constants::Embedding::DocumentPrefix.empty())
		instructions = llama::tokenize(pVocab, toStr(Constants::Embedding::DocumentPrefix), false);
	tokens.insert(tokens.begin(), instructions.begin(), instructions.end());

	llama_batch batch;
	if (!llama::init_embedding_batch(_pModel, _pCtx, tokens, batch))
		return false;

	if (batch.n_tokens > ctx_size)
		batch.n_tokens = ctx_size;
	batch.logits[batch.n_tokens - 1] = true;

	std::vector<float> embeddings(n_embed, 0);
	float * embedding = embeddings.data();
	batch_decode(_pCtx, batch, embedding, 1, (int32_t)n_embed, 2);
	llama_batch_free(batch);

	out_embedding = EmbeddingVector {
		/*modelName*/ _modelName,
		/*content*/ content,
		std::vector<float>(embedding, embedding + n_embed),
	};
	return true;
}

void LLMEmbedding::CompareSimilarity(const std::vector<float>& vec, size_t n_sentences)
{
	auto& embeddings = Embeddings::GetEmbeddings();

	std::vector<std::pair<float, fig::string>> results;
	const size_t MaxLength = 64;
	for (size_t i = 0; i < embeddings.size(); ++i)
	{
		if (embeddings[i].vec.size() != vec.size())
			continue;

		float similarity = fig::llm::util::embd_similarity_cos(embeddings[i].vec, vec, (int32_t)vec.size());

		fig::string content = embeddings[i].content;
		if (content.size() > MaxLength)
			content = content.substr(0, MaxLength) + "...";
		results.push_back(std::make_pair(similarity, content));
	}
	std::sort(results.begin(), results.end(), [](auto a, auto b) {
		return a.first > b.first;
	});

	for (int i = 0; i < results.size() && i < 5; ++i)
		LogLn(std::format("Similarity {0:.3f} = \"{1}\"", results[i].first, results[i].second));
}