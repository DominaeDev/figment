#pragma once

#include "Types.h"
#include <llama.h>

enum class Role
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
	Bot5,
	Bot6,
	Bot7,
	Bot8,
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

struct ModelState
{
	llama_model* pModel = nullptr;
	const llama_vocab* pVocab = nullptr;
	llama_context* pCtx = nullptr;
	llama_sampler* pSampler = nullptr;
	llama_sampler* pActiveGrammar = nullptr;
	std::map<GrammarFlag, llama_sampler*> grammars = {};

	string modelName {};
	std::mt19937 rng {};

	void Release();

	bool HasGrammar(GrammarFlag flags) const;
	llama_sampler* SetActiveGrammar(GrammarFlag flags);
};

enum class ContextBlockFlag : int32_t
{
	None		= 0,
	Cached		= 1 << 0,
	Volatile	= 1 << 1,
};
DEFINE_ENUM_FLAGS(ContetBlockFlag, int32_t);

struct ContextBlock 
{
	string responseId;
	Role role;
	string name;
    string content;
	std::vector<int32_t> tokens;
	int32_t offset {};
	bool cached = false;
	int ttl = -1;

	int32_t length() const { return toI(tokens.size()); }
};

struct ContextState
{
	llama_context* pCtx = nullptr;			// Non-owned
	const llama_vocab* pVocab = nullptr;	// Non-owned
	llama_batch batch {};					// Representation of the kv-cache (mirror)
	std::vector<int32_t> system_tokens;
	std::map<Role, std::vector<int32_t>> personas;
	std::vector<ContextBlock> blocks;

	int32_t persona_pos = 0;				// persona insertion point
	int32_t response_pos = 0;				// start of response, including prompt template preamble
	int32_t prepend_pos = 0;				// start of response, not including prompt template preamble
	int32_t blocks_pos = 0;					// position of first message block
	int32_t current_pos = 0;				// current position (cursor)

	Role activePersona = Role::Undefined;
	
	int32_t AssignBlockPositions();
};

enum class LLMOption : int32_t
{
	None = 0,
	UseCharacterIds			= 1 << 0,
	AllowUserResponse		= 1 << 1,
	LimitMessages			= 1 << 2,
	RandomizeMessageCount	= 1 << 3,
	GreetUser				= 1 << 4,
	SwapPersonas			= 1 << 5,
	Uncensored				= 1 << 6,
	StateVariables			= 1 << 7,
	Embeddings				= 1 << 8,
};
DEFINE_ENUM_FLAGS(LLMOption, int32_t)

template <typename E>
inline constexpr bool CheckEnumFlag(const E set, const E flag)
{
	return (set & flag) == flag;
}

struct Sentence
{
	Role role;
	string sentence;
};
using Sentences = std::vector<Sentence>;