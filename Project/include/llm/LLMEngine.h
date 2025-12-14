#ifndef LLM_ENGINE_H__
#define LLM_ENGINE_H__
#pragma once

#include "llm/LLMTypes.h"
#include "llm/ModelState.h"
#include "llm/LLMStatus.h"

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

	class LLMEngine
	{
	public:
		LLMEngine();
		~LLMEngine() = default;

		bool Initialize(fig::string modelFilename, fig::string embeddingFilename, LoadModelProgressCallback onProgress, LoadModelCallback onComplete);
		bool Shutdown();

		bool IsInitialized() const { return _readyState.load() == ReadyState::Ready; }
		bool IsInitializing() const { return _readyState.load() == ReadyState::Initializing; }

		std::shared_ptr<LLMStatusChannel> GetStatusChannel() noexcept { return _pStatus; }
		LLMInstancePtr CreateInstance(int32_t ctx_size, bool embeddings);
		bool DestroyInstance(LLMInstancePtr instance);

	private:
		using __LoadModelCallback = std::function<void(std::shared_ptr<ModelState>)>;
		void __LoadModel(fig::string modelFilename, fig::string embeddingFilename, __LoadModelCallback onComplete);
		static bool OnLoadModelProgress(float progress, void* user_data);

		void SetReadyState(ReadyState readyState);

	private:
		std::atomic<ReadyState> _readyState { ReadyState::Uninitialized };
		std::mutex _stateMutex; // Guards ready state

		std::unique_ptr<std::jthread> _workerThread;
		std::shared_ptr<ModelState> _modelState {};
		std::shared_ptr<LLMStatusChannel> _pStatus {};

		std::vector<LLMInstancePtr> _instances {};
		LLMInstancePtr _mainInstance {};

		LoadModelProgressCallback _pLoadModelProgressCallback = nullptr;
	public:
		std::atomic<int64_t> usedVRAM; // As reported from llama.cpp
		std::atomic<int64_t> usedRAM; // As reported from llama.cpp
	};
}

#endif
