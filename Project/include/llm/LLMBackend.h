#ifndef LLM_ENGINE_H__
#define LLM_ENGINE_H__
#pragma once

#include "llm/LLMTypes.h"
#include "llm/ModelState.h"
#include "llm/LLMStatus.h"
#include "data/ModelSettings.h"

#include <functional>
#include <thread>
#include <mutex>

namespace fig::llm
{
	using LoadModelCallback = std::function<void(bool)>;
	using LoadModelProgressCallback = std::function<void(int)>;

	class ModelState;
	class LLMInstance;
	class LLMEmbedding;

	using LLMObserverCallback = std::function<void(const LLMStatus&)>;
	using LLMObserverCallbackId = uint32_t;

	class LLMBackend
	{
	public:
		LLMBackend();
		~LLMBackend() = default;

		bool Initialize(const fig::data::ModelSettings& settings, LoadModelProgressCallback onProgress, LoadModelCallback onComplete);
		bool Shutdown();

		bool IsInitialized() const { return _readyState.load() == ReadyState::Ready; }
		bool IsInitializing() const { return _readyState.load() == ReadyState::Initializing; }

		std::shared_ptr<LLMStatusChannel> GetStatusChannel() noexcept { return _pStatus; }
		LLMInstancePtr CreateInstance();
		bool DestroyInstance(LLMInstancePtr instance);

		LLMObserverCallbackId RegisterObserver(LLMObserverCallback fnCallback);
		void UnregisterObserver(LLMObserverCallbackId id);
		void Update(float fElapsed);

	private:
		using __LoadModelCallback = std::function<void(std::shared_ptr<ModelState>)>;
		void __LoadModel(fig::data::ModelSettings settings, __LoadModelCallback onComplete);
		static bool OnLoadModelProgress(float progress, void* user_data);

		void SetReadyState(ReadyState readyState);
		void PollStatus();

	private:
		std::atomic<ReadyState> _readyState { ReadyState::Uninitialized };
		std::mutex _stateMutex; // Guards ready state

		std::unique_ptr<std::jthread> _workerThread;
		std::shared_ptr<ModelState> _modelState {};
		std::shared_ptr<LLMStatusChannel> _pStatus {};
		std::list<LLMStatus> _statusCache {};

		std::vector<LLMInstancePtr> _instances {};
		LoadModelProgressCallback _pLoadModelProgressCallback = nullptr;
		float _fPollingCounter = 0.0f;

		std::unordered_map<LLMObserverCallbackId, LLMObserverCallback> _observers {};
		LLMObserverCallbackId _nextId = 0;
	public:
		std::atomic<int64_t> usedVRAM; // As reported from llama.cpp
		std::atomic<int64_t> usedRAM; // As reported from llama.cpp
	};
}

#endif
