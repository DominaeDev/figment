#pragma once

#include "llm/LLMTypes.h"
#include "llm/ModelState.h"
#include "llm/LLMEmbedding.h"
#include "llm/LLMStateVariables.h"
#include "llm/LLMStatus.h"
#include "llm/LLMContext.h"
#include "chat/ChatSession.h"

#include <thread>
#include <mutex>

namespace fig::llm
{
	class LLMStatusChannel;

	struct MessagePiece
	{
		fig::uuid responseId;		// response block
		fig::uuid subMessageId;		// shared id for pieces of the same message type
		fig::string identifier {};	// who said this?
		fig::string content {};
		fig::chat::Role role = fig::chat::Role::Undefined;
		fig::chat::MessageType msgType = fig::chat::MessageType::Undefined;
		bool isComplete = false;
	};

	struct RemovedMessage
	{
		fig::uuid responseId;
		fig::string content;
		fig::chat::Role role;
	};

	struct LLMChatArguments
	{
		fig::chat::ChatSession session;
		fig::chat::Messages messages;
		fig::chat::ChatOptions options;
		int32_t narrationCooldownDuration = Constants::Chat::DefaultNarratorCooldown;
	};

	enum class LLMTaskFlag : int32_t
	{
		None = 0,
		HiddenMessage = 1 << 0,
		ExtendMessage = 1 << 1,
	};
	using LLMTaskFlags = EnumFlags<LLMTaskFlag>;

	class LLMBackend;

	class LLMInstance
	{
		friend class LLMBackend;
	public:
		LLMInstance();
		~LLMInstance();

		bool Initialize(LLMChatArguments args);
		void Shutdown();

		bool IsInitialized() const;
		bool IsReady() const;
		bool IsGenerating() const;

		bool Continue(fig::uuid responseId, fig::uuid subMessageId, bool extend);
		bool Halt();

		// Tasks
		bool GreetUser();
		bool SendMessage(fig::string message);
		bool PushMessage(fig::chat::Role role, const fig::string& message, fig::chat::MessageType msgType = fig::chat::MessageType::Undefined, bool visible = true, int ttl = 0);
		bool Instigate(fig::chat::Role role, fig::chat::MessageType msgType, int messageCount = 0);
		bool Instruct(fig::string instructions);
		std::vector<RemovedMessage> EraseMessages(int numMessages = 1);
		std::vector<RemovedMessage> RollbackUserMessage();

		bool ResetChat(int seed = -1);
		bool Reseed(uint32_t seed = 0xFFFFFFFF);
		int32_t RewindTime(int32_t rewind_turns);
		std::set<fig::uuid> GetActiveMessages();

		bool SetStateVariable(fig::string name, fig::string value, bool allowCreate = true);
		bool PollResponse(MessagePiece& piece);

		void DumpContext() const;
#if _DEBUG
		bool GenerateEmbedding(fig::string text);
#endif

		const fig::chat::ChatSession& GetSession() const { return _session; }
		std::map<fig::string, fig::string> GetStateVariables();

	private:
		void ClearResponseQueue();
		bool CanGenerate() const;
		bool PushDirection(const fig::string& message, int32_t ttl = 1);
		bool PushNarration(const fig::string& message);
		bool PushUserDialogue(const fig::string& message);
		bool PushUserMessage(const fig::string& message); // User-facing
	public:
		enum class GenerateFlag : int32_t
		{
			None = 0,
			Generate = 1 << 0,
			Continuation = 1 << 1,
			Instigation = 1 << 2,
			AllowNarrator = 1 << 3,
		};
		using GenerateFlags = EnumFlags<GenerateFlag>;

	private:
		struct __PartialResult
		{
			fig::string piece;
			fig::string fullText;
		};

		enum class InternalError : int {
			NoError = 0,
			ContextFull,
			InvalidContextError,
			DecodeError,
			SamplerError,
			GrammarError,
			PersonaSwapError,
			UnknownError,
		};

		using __PartialResultCallback = std::function<void(__PartialResult)>;
		using __GenerationCompleteCallback = std::function<void(InternalError error, fig::string msg)>;

		struct PrepareArguments
		{
			fig::chat::Role responder = fig::chat::Role::Bot1;
			bool isContinuation = false;
			bool progressTime = true;
		};
		InternalError __PrepareGeneration(PrepareArguments args);

		struct GenerateArguments
		{
			fig::chat::Role role = fig::chat::Role::Undefined;
			fig::chat::MessageType msgType = fig::chat::MessageType::Undefined;
			GenerateFlags flags = GenerateFlags::None;
			fig::chat::ChatOptions::GroupChatMode groupChatMode {};
			int maxMessages = 0;
			fig::string prepend {};
			fig::uuid responseId {};
			fig::uuid subMessageId {};
			fig::chat::Sentences history; // Used for embedding
		};
		void __Generate(std::stop_token& stop, GenerateArguments args, __GenerationCompleteCallback onComplete);
		void StartGeneration();
		bool SwapPersona(fig::chat::Role persona, bool immediate = true);

		void RefreshActiveResponses();
		bool RebuildKVCache();

		fig::chat::Sentences GetHistory(size_t depth);
		SamplerPtr CompileGrammar(GrammarFlags grammarFlags);
		void InitSamplers();

		// Tasks
		enum class LLMTaskType
		{
			Undefined = 0,
			SendMessage,
			PushMessage,
			Instigate,
			Continue,
		};

		struct LLMTask
		{
			LLMTaskType type { LLMTaskType::Undefined };

			// Parameters
			fig::string input;
			fig::chat::Role role = fig::chat::Role::Undefined;
			fig::chat::MessageType msgType = fig::chat::MessageType::Undefined;
			fig::uuid responseId {};
			fig::uuid subMessageId {};

			LLMTaskFlags flags = LLMTaskFlags::None;
			int msgCount = 0;
			int ttl = 0;
		};
		bool EnqueueTask(LLMTask task);
		bool ClearTasksQueue();

		void __ProcessTaskQueue(std::stop_token stop, __GenerationCompleteCallback onComplete);
		bool __ExectuteNextTask(PrepareArguments& prepareArgs, GenerateArguments& generateArgs);
		bool __SendMessage(fig::string message, PrepareArguments& prepareArgs, GenerateArguments& generateArgs);
		bool __PushMessage(fig::chat::Role role, fig::string message, fig::chat::MessageType msgType, bool visible, int32_t ttl);
		bool __Instigate(fig::chat::Role role, fig::chat::MessageType msgType, int messageCount, PrepareArguments& prepareArgs, GenerateArguments& generateArgs);
		bool __Continue(fig::uuid responseId, fig::uuid subMessageId, bool extend, PrepareArguments& prepareArgs, GenerateArguments& generateArgs);

		void Panic(InternalError error, const fig::string& message);

	private:
		void SetReadyState(ReadyState readyState);
		std::atomic<ReadyState> _readyState { ReadyState::Uninitialized };

		std::timed_mutex _stateMutex; // Guards state variables
		ModelState _modelState {};
		LLMContext _contextState;

		std::mutex _resultMutex; // Guards output queue
		std::queue<MessagePiece> _resultQueue;
		std::set<fig::uuid> _activeResponseIds;

		std::unique_ptr<std::jthread> _workerThread;
		std::shared_ptr<LLMStatusChannel> _pStatus;

		// Tasks
		std::mutex _taskMutex; // Guards task queue
		std::queue<LLMTask> _tasks;

		// Session
		fig::chat::ChatSession _session;
		fig::chat::ChatOptions _options;
		std::atomic<int32_t> _turn_counter = 0;

		// State
		LLMStateVariables _stateVars;
		int32_t _narratorCooldownDuration = 0;
		int32_t _next_narrator_turn = 0;
	};
}