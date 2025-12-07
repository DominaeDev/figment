export module LLMTypes;

export import <llama.h>;
export import Types;

export
{
	enum class Role : int32_t
	{
		Undefined = 0,
		System,
		Narrator,
		Director,
		User,

		Bot1 = 10,
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

	using ModelPtr = llama_model*;
	using ContextPtr = llama_context*;
	using VocabPtr = const llama_vocab*;
	using SamplerPtr = llama_sampler*;
	using Batch = llama_batch;
	using Token = llama_token;

	enum class LLMOption : uint32_t
	{
		UseCharacterIds = 1 << 0,
		AllowUserResponse = 1 << 1,
		LimitMessages = 1 << 2,
		RandomizeMessageCount = 1 << 3,
		GreetUser = 1 << 4,
		Embeddings = 1 << 5,
		Uncensored = 1 << 6,

		StateVariables = 1 << 7,
		ReportStateChanges = 1 << 8,

		UseMultipleSequences = 1 << 9,
	};
	using LLMOptions = EnumFlags<LLMOption>;

	enum class Sequence : int32_t
	{
		None = 0,
		Bot1 = 1 << 0,
		Bot2 = 1 << 1,
		Bot3 = 1 << 2,
		Bot4 = 1 << 3,
		Shared = Bot1 | Bot2 | Bot3 | Bot4,
		Default = Bot1,
	};
	using SequenceId = EnumFlags<Sequence>;

	using SequenceIndices = std::vector<llama_seq_id>;

	struct Sentence
	{
		Role role;
		string sentence;
	};
	using Sentences = std::vector<Sentence>;
}