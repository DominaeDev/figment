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
		ChatSession session;
		Messages messages;
		ChatOptions options;
		int32_t narrationCooldownDuration = Constants::Chat::DefaultNarratorCooldown;
	};

	enum class LLMTaskFlag : int32_t
	{
		None = 0,
		HiddenMessage = 1 << 0,
	};
	using LLMTaskFlags = EnumFlags<LLMTaskFlag>;

	class LLMEngine;

	class LLMInstance
	{
		friend class LLMEngine;
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

		bool ResetChat(int seed = -1);
		bool Reseed(uint32_t seed = 0xFFFFFFFF);
		std::vector<RemovedMessage> RemoveMessages(int numMessages = 1, bool rewindTime = true);
		std::vector<RemovedMessage> RollbackUserMessage();
		std::set<fig::string> GetActiveMessages();

		bool SetStateVariable(fig::string name, fig::string value, bool allowCreate = true);
		bool PollResponse(MessagePiece& piece);

		void DumpSequence(int32_t seq_id) const;
		void DumpContext() const;
#if _DEBUG
		bool GenerateEmbedding(fig::string text);
#endif

		const ChatSession& GetSession() const { return _session; }
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
			SwapPersonas = 1 << 4,
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
			DecodeError,
			SamplerError,
			GrammarError,
			UnknownError,
		};

		using __PartialResultCallback = std::function<void(__PartialResult)>;
		using __GenerationCompleteCallback = std::function<void(InternalError error, fig::string msg)>;

		struct PrepareArguments
		{
			Role responder = Role::Bot1;
			bool isContinuation = false;
			int time = 0;	// decrement ttl
		};
		void __PrepareGeneration(PrepareArguments args);

		struct GenerateArguments
		{
			Role role = Role::Undefined;
			MessageType msgType = MessageType::Undefined;
			GenerateFlags flags = GenerateFlags::None;
			int maxMessages = 0;
			fig::string prepend {};
			fig::string responseId {};
			fig::string subMessageId {};
			Sentences history; // Used for embedding
		};
		void __Generate(std::stop_token& stop, GenerateArguments args, __GenerationCompleteCallback onComplete);
		void StartGeneration();
		bool SwapPersona(Role persona);

		void RefreshActiveResponses();
		std::vector<RemovedMessage> impl_RemoveMessages(int numMessages, bool rewindTime);
		bool RebuildKVCache();

		Sentences GetHistory(size_t depth);
		SamplerPtr CompileGrammar(GrammarFlags grammarFlags);
		void InitSamplers();

		// Tasks
		enum class LLMTaskType
		{
			SendMessage,
			PushMessage,
			Instigate,
			Continue,
		};

		struct LLMTask
		{
			LLMTaskType type;

			// Parameters
			fig::string input;
			Role role = Role::Undefined;
			MessageType msgType = MessageType::Undefined;

			LLMTaskFlags flags = LLMTaskFlags::None;
			int msgCount = 0;
			int ttl = 0;
		};
		bool EnqueueTask(LLMTask task);
		bool ClearTasksQueue();

		void __ProcessTaskQueue(std::stop_token stop, __GenerationCompleteCallback onComplete);
		bool __ExectuteNextTask(PrepareArguments& prepareArgs, GenerateArguments& generateArgs);
		bool __SendMessage(fig::string message, PrepareArguments& prepareArgs, GenerateArguments& generateArgs);
		bool __PushMessage(Role role, fig::string message, MessageType msgType, bool visible, int ttl);
		bool __Instigate(Role role, MessageType msgType, int messageCount, PrepareArguments& prepareArgs, GenerateArguments& generateArgs);

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
		ChatSession _session;
		ChatOptions _options;
		bool _bCtxReallocateNextTurn = false;

		// State
		LLMStateVariables _stateVars;
		int32_t _narratorCooldownDuration = 0;
		int32_t _narratorCooldown = 0;
	};
}