#pragma once

#include "llm/LLMTypes.h"
#include "llm/ModelState.h"
#include "llm/LLMEmbedding.h"
#include "llm/LLMStateVariables.h"
#include "llm/LLMStatus.h"
#include "llm/Context.h"
#include "Constants.h"
#include "model/ChatSession.h"

#include <vector>
#include <set>
#include <map>
#include <functional>
#include <thread>
#include <mutex>
#include <queue>
#include <array>

namespace fig::llm
{
	class LLMStatusChannel;

	struct MessagePiece
	{
		fig::string responseId;		// response block
		fig::string subMessageId;	// shared id for pieces of the same message type
		fig::string identifier {};	// who said this?
		fig::string content {};
		Role role = Role::Undefined;
		MessageType msgType = MessageType::Undefined;
		bool isComplete = false;
	};

	struct RemovedMessage
	{
		fig::string responseId;
		fig::string content;
		Role role;
	};

	struct LLMChatArguments
	{
		fig::io::ChatSession session;
		Messages messages;
		ChatOptions options;
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

		bool Continue(fig::string responseId, fig::string subMessageId, bool extend);
		bool Halt();

		// Tasks
		bool GreetUser();
		bool SendMessage(fig::string message);
		bool PushMessage(Role role, fig::string message, MessageType msgType = MessageType::Undefined, bool visible = true, int ttl = 0);
		bool Instigate(Role role, MessageType msgType, int messageCount = 0);
		bool Instruct(fig::string instructions);
		std::vector<RemovedMessage> EraseMessages(int numMessages = 1);
		std::vector<RemovedMessage> RollbackUserMessage();

		bool ResetChat(int seed = -1);
		bool Reseed(uint32_t seed = 0xFFFFFFFF);
		int32_t RewindTime(int32_t rewind_turns);
		std::set<fig::string> GetActiveMessages();

		bool SetStateVariable(fig::string name, fig::string value, bool allowCreate = true);
		bool PollResponse(MessagePiece& piece);

		void DumpContext() const;
#if _DEBUG
		bool GenerateEmbedding(fig::string text);
#endif

		const fig::io::ChatSession& GetSession() const { return _session; }
		std::map<fig::string, fig::string> GetStateVariables();

	private:
		void ClearResponseQueue();
		bool CanGenerate() const;

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
			Role responder = Role::Bot1;
			bool isContinuation = false;
			bool progressTime = true;
		};
		InternalError __PrepareGeneration(PrepareArguments args);

		struct GenerateArguments
		{
			Role role = Role::Undefined;
			MessageType msgType = MessageType::Undefined;
			GenerateFlags flags = GenerateFlags::None;
			ChatOptions::GroupChatMode groupChatMode {};
			int maxMessages = 0;
			fig::string prepend {};
			fig::string responseId {};
			fig::string subMessageId {};
			Sentences history; // Used for embedding
		};
		void __Generate(std::stop_token& stop, GenerateArguments args, __GenerationCompleteCallback onComplete);
		void StartGeneration();
		bool SwapPersona(Role persona, bool immediate = true);

		void RefreshActiveResponses();
		bool RebuildKVCache();

		Sentences GetHistory(size_t depth);
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
			Role role = Role::Undefined;
			MessageType msgType = MessageType::Undefined;
			fig::string responseId {};
			fig::string subMessageId {};

			LLMTaskFlags flags = LLMTaskFlags::None;
			int msgCount = 0;
			int ttl = 0;
		};
		bool EnqueueTask(LLMTask task);
		bool ClearTasksQueue();

		void __ProcessTaskQueue(std::stop_token stop, __GenerationCompleteCallback onComplete);
		bool __ExectuteNextTask(PrepareArguments& prepareArgs, GenerateArguments& generateArgs);
		bool __SendMessage(fig::string message, PrepareArguments& prepareArgs, GenerateArguments& generateArgs);
		bool __PushMessage(Role role, fig::string message, MessageType msgType, bool visible, int32_t ttl);
		bool __Instigate(Role role, MessageType msgType, int messageCount, PrepareArguments& prepareArgs, GenerateArguments& generateArgs);
		bool __Continue(fig::string responseId, fig::string subMessageId, bool extend, PrepareArguments& prepareArgs, GenerateArguments& generateArgs);

		void Panic(InternalError error, const fig::string& message);

	private:
		void SetReadyState(ReadyState readyState);
		std::atomic<ReadyState> _readyState { ReadyState::Uninitialized };

		std::timed_mutex _stateMutex; // Guards state variables
		ModelState _modelState {};
		Context _contextState;

		std::mutex _resultMutex; // Guards output queue
		std::queue<MessagePiece> _resultQueue;
		std::set<fig::string> _activeResponseIds;

		std::unique_ptr<std::jthread> _workerThread;
		std::shared_ptr<LLMStatusChannel> _pStatus;

		// Tasks
		std::mutex _taskMutex; // Guards task queue
		std::queue<LLMTask> _tasks;

		// Session
		fig::io::ChatSession _session;
		ChatOptions _options;
		std::atomic<int32_t> _turn_counter = 0;

		// State
		LLMStateVariables _stateVars;
		int32_t _narratorCooldownDuration = 0;
		int32_t _next_narrator_turn = 0;
	};
}