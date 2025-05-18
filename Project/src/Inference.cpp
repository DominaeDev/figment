#include "llama.h"
#include "Inference.h"
#include <vector>

#pragma comment(lib, "ggml-base.lib")

llama_model* Inference::_pModel = nullptr;
const llama_vocab* Inference::_pVocab = nullptr;
llama_context* Inference::_pCtx = nullptr;
llama_sampler* Inference::_pSampler = nullptr;

void Inference::Initialize()
{
	// only print errors
	llama_log_set([](enum ggml_log_level level, const char* text, void* /* user_data */) {
		if (level >= GGML_LOG_LEVEL_ERROR)
		{
			fprintf(stderr, "%s", text);
		}
	}, nullptr);

	// load dynamic backends
	ggml_backend_load_all();
}

void Inference::Shutdown()
{
    llama_sampler_free(_pSampler);
    llama_free(_pCtx);
    llama_model_free(_pModel);
}

bool Inference::LoadModel(string filename)
{
	int ngl = 99;
	int n_ctx = 2048;

	// initialize the model
	llama_model_params model_params = llama_model_default_params();
	model_params.n_gpu_layers = ngl;

	_pModel = llama_model_load_from_file(filename.c_str(), model_params);
	if (!_pModel)
	{
		fprintf(stderr, "%s: error: unable to load model\n", __func__);
		return false;
	}
	_pVocab = llama_model_get_vocab(_pModel);

	// initialize the context
	llama_context_params ctx_params = llama_context_default_params();
	ctx_params.n_ctx = n_ctx;
	ctx_params.n_batch = n_ctx;

	_pCtx = llama_init_from_model(_pModel, ctx_params);
	if (!_pCtx)
	{
		fprintf(stderr, "%s: error: failed to create the llama_context\n", __func__);
		return false;
	}

	// initialize the sampler
    _pSampler = llama_sampler_chain_init(llama_sampler_chain_default_params());
    llama_sampler_chain_add(_pSampler, llama_sampler_init_min_p(0.05f, 1));
    llama_sampler_chain_add(_pSampler, llama_sampler_init_temp(0.8f));
    llama_sampler_chain_add(_pSampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

	return true;
}

bool Inference::Generate(const string& prompt, string& outResponse)
{
    std::string response;

    const bool is_first = llama_kv_self_used_cells(_pCtx) == 0;

    // tokenize the prompt
    const int32_t n_prompt_tokens = -llama_tokenize(_pVocab, prompt.c_str(), (int32_t)prompt.size(), NULL, 0, is_first, true);
    std::vector<llama_token> prompt_tokens(n_prompt_tokens);
    if (llama_tokenize(_pVocab, prompt.c_str(), (int32_t)prompt.size(), prompt_tokens.data(), (int32_t)prompt_tokens.size(), is_first, true) < 0) {
        GGML_ABORT("failed to tokenize the prompt\n");
		return false;
    }

    // prepare a batch for the prompt
    llama_batch batch = llama_batch_get_one(prompt_tokens.data(), (int32_t)prompt_tokens.size());
    llama_token new_token_id;
    while (true) {
        // check if we have enough space in the context to evaluate this batch
        int n_ctx = llama_n_ctx(_pCtx);
        int n_ctx_used = llama_kv_self_used_cells(_pCtx);
        if (n_ctx_used + batch.n_tokens > n_ctx) {
            printf("\033[0m\n");
            fprintf(stderr, "context size exceeded\n");
			return false;
        }

        if (llama_decode(_pCtx, batch)) {
            GGML_ABORT("failed to decode\n");
        }

        // sample the next token
        new_token_id = llama_sampler_sample(_pSampler, _pCtx, -1);

        // is it an end of generation?
        if (llama_vocab_is_eog(_pVocab, new_token_id)) {
            break;
        }

        // convert the token to a string, print it and add it to the response
        char buf[256];
        int n = llama_token_to_piece(_pVocab, new_token_id, buf, sizeof(buf), 0, true);
        if (n < 0) {
            GGML_ABORT("failed to convert token to piece\n");
			return false;
        }
        std::string piece(buf, n);
        printf("%s", piece.c_str());
        fflush(stdout);
        response += piece;

        // prepare the next batch with the sampled token
        batch = llama_batch_get_one(&new_token_id, 1);
    }

	outResponse = response;
    return true;
};

bool Inference::SendMessage(string name, string message, string& outResponse)
{
	std::vector<llama_chat_message> messages;
	std::vector<char> formatted(llama_n_ctx(_pCtx));
	int prev_len = 0;

	const char* tmpl = llama_model_chat_template(_pModel, /* name */ nullptr);

	// add the user input to the message list and format it
	messages.push_back({ _strdup(name.c_str()), _strdup(message.c_str()) });

	int new_len = llama_chat_apply_template(tmpl, messages.data(), (int32_t)messages.size(), true, formatted.data(), (int32_t)formatted.size());
	if (new_len > (int)formatted.size())
	{
		formatted.resize(new_len);
		new_len = llama_chat_apply_template(tmpl, messages.data(), (int32_t)messages.size(), true, formatted.data(), (int32_t)formatted.size());
	}
	if (new_len < 0)
	{
		fprintf(stderr, "failed to apply the chat template\n");
		return false;
	}

	// remove previous messages to obtain the prompt to generate the response
	std::string prompt(formatted.begin() + prev_len, formatted.begin() + new_len);

	// generate a response
	if (!Generate(prompt, outResponse))
		return false;

	// add the response to the messages
	messages.push_back({ "assistant", _strdup(outResponse.c_str()) });
	prev_len = llama_chat_apply_template(tmpl, messages.data(), messages.size(), false, nullptr, 0);
	if (prev_len < 0)
	{
		fprintf(stderr, "failed to apply the chat template\n");
		return false;
	}
	
	return true;
}