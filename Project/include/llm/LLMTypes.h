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

enum class MessageType
{
	Undefined = 0, 

	Dialogue,
	Action,
	Thought,

	SystemMessage,
	Narration,
	Direction,
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

enum class Responder
{ 
	None, 
	User, 
	Narrator, 
	Director, 
	Bot 
};


enum class Grammar
{
	None				= 0,
	Default				= 1,
	StubDialogue		= 2,
	StubAction			= 3,
	StubNarration		= 4,
	ContinueDialogue	= 5,
	ContinueAction		= 6,
	ContinueNarration	= 7,
};

struct ModelState
{
	llama_model* pModel = nullptr;
	llama_context* pCtx = nullptr;
	llama_sampler* pSampler = nullptr;
	llama_sampler* pActiveGrammar = nullptr;
	std::array<llama_sampler*, 8> grammars = {};

	string modelName {};
	std::mt19937 rng {};

	void Release();

	llama_sampler* SetActiveGrammar(Grammar grammar);
};

struct ContextBlock 
{
	string responseId;
	Role role;
	string name;
    string content;
	std::vector<int32_t> tokens;
	int32_t offset;
	bool cached = false;
	int ttl = -1;

	int32_t length() const { return toI(tokens.size()); }
};

struct ContextState
{
	llama_batch batch {};			// Representation of the kv-cache (mirror)
	std::vector<int32_t> system_tokens;
	std::map<Role, std::vector<int32_t>> personas;
	std::vector<ContextBlock> blocks;
	int32_t persona_pos = 0;		// persona insertion point
	int32_t response_pos = 0;		// start of response
	int32_t prepend_pos = 0;
	int32_t blocks_pos = 0;			// chat start
	int32_t current_pos = 0;		// cursor position

	int32_t AssignBlockPositions();

	Role activePersona = Role::Undefined;
};