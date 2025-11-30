#pragma once

#include "Types.h"
#include <llama.h>

enum class Role : int32_t
{
	Undefined	= 0,
	System,
	Narrator,
	Director,
	User,

	Bot1		= 10,
	Bot2,
	Bot3,
	Bot4,
	Bot5,	// ?
	Bot6,	// ?
	Bot7,	// ?
	Bot8,	// ?
};

constexpr inline bool is_bot(Role role) { return role >= Role::Bot1 && role <= Role::Bot8; }
constexpr inline bool is_npc(Role role) { return role == Role::Director || role == Role::Narrator || role == Role::System; }
constexpr inline int32_t get_bot_index(Role role) { return is_bot(role) ? static_cast<int32_t>(role) - static_cast<int32_t>(Role::Bot1) : -1; }
constexpr inline Role bot_from_index(int32_t botIndex)
{
    constexpr int32_t first = (int32_t)Role::Bot1;
    constexpr int32_t last = (int32_t)Role::Bot8;
    if (first + botIndex < 0 || first + botIndex > last)
        return Role::Undefined;
    return static_cast<Role>(first + botIndex);
}

enum class MessageType
{
	Undefined = 0, 

	Dialogue,
	Action,
	Thought,

	SystemMessage,
	Narration,
	Direction,

	StateReport,
};

struct Message 
{
	Role role;
    string content;
    string name;
};
using Messages = std::vector<Message>;

struct Submessage
{
	MessageType msgType = MessageType::Undefined;
	string content;
};

enum class GrammarFlag : int32_t
{
	None			= 0,

	// Generation type
	Default			= 1 << 0,
	Stub			= 1 << 1,
	Continue		= 1 << 2,

	// Message type
	Talk			= 1 << 3,
	Act				= 1 << 4,
	Narrate			= 1 << 5,

	// Options
	EnableNarrator	= 1 << 6,
	EnableState		= 1 << 7,
	UseCharacterIds	= 1 << 8,
	AllowUser		= 1 << 9,
};
DEFINE_ENUM_FLAGS(GrammarFlag, int32_t)

using ModelPtr = llama_model*;
using ContextPtr = llama_context*;
using VocabPtr = const llama_vocab*;
using SamplerPtr = llama_sampler*;
using Batch = llama_batch;
using Token = llama_token;

struct ModelState
{
	ModelPtr pModel = nullptr;
	VocabPtr pVocab = nullptr;
	ContextPtr pCtx = nullptr;
	SamplerPtr pSampler = nullptr;
	SamplerPtr pActiveGrammar = nullptr;
	std::map<GrammarFlag, SamplerPtr> grammars {};

	string modelName {};
	std::mt19937 rng {};
	int32_t num_sequences {};
	int32_t ctx_size {};

	void Release();

	bool HasGrammar(GrammarFlag flags) const;
	llama_sampler* SetActiveGrammar(GrammarFlag flags);
};

enum class LLMOption : uint32_t
{
	None = 0,
	UseCharacterIds			= 1 << 0,
	AllowUserResponse		= 1 << 1,
	LimitMessages			= 1 << 2,
	RandomizeMessageCount	= 1 << 3,
	GreetUser				= 1 << 4,
	Embeddings				= 1 << 5,
	Uncensored				= 1 << 6,
	
	StateVariables			= 1 << 7,
	ReportStateChanges		= 1 << 8,

	UseMultipleSequences	= 1 << 9,
};
DEFINE_ENUM_FLAGS(LLMOption, uint32_t)

struct Sentence
{
	Role role;
	string sentence;
};
using Sentences = std::vector<Sentence>;

enum class SequenceId : int32_t
{
	None	= 0,
	Bot1	= 1 << 0,
	Bot2	= 1 << 1,
	Bot3	= 1 << 2,
	Bot4	= 1 << 3,
	Shared	= Bot1 | Bot2 | Bot3 | Bot4,
	Default = Bot1,
};
DEFINE_ENUM_FLAGS(SequenceId, int32_t);

using SequenceIndices = std::vector<llama_seq_id>;