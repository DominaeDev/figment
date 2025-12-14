#ifndef STATUS_SIGNAL_H__
#define STATUS_SIGNAL_H__
#pragma once

#include "Types.h"
#include <queue>
#include <mutex>
#include <optional>

enum class LLMStatusSignal : uint32_t
{
	Nothing = 0,
	ModelLoading,
	ModelLoaded,
	ModelLoadFailure,
	ModelUnloaded,
	ModelUnloadRequest,

	ChatInitializing,
	ChatInitialized,
	ChatInitializationFailure,

	GenerationStarted,
	GenerationComplete,

	CompletedMessage,

	RebuildingKVCache,
};

enum class ReadyState : uint32_t
{ 
	Invalid,			// Invalid state
	LoadError,			// Failed to load model
	Uninitialized,		// Model is not loaded
	LoadingModel,		// Model is loading
	ModelLoaded,		// Model is loaded, chat uninitialized
	Initializing,		// Initializing chat
	Ready,				// Chat is initialized and ready
	Generating,			// Chat is generating
	RebuildingKVCache,	// Chat is rebuilding the kv cache
};

struct LLMStatus
{
	string modelName;
	int32_t allocCtxSize = 0;
	int32_t usedCtxSize = 0;
	int64_t usedVRAM = 0;
	int64_t usedRAM = 0;
	ReadyState readyState { ReadyState::Uninitialized };
	double tokensPerSec = 0.0;
	LLMStatusSignal signal;

	inline bool IsReady() const noexcept { return readyState >= ReadyState::Ready; }
};

class LLMStatusChannel
{
public:
	LLMStatus PollStatus();

	void EmitSignal(LLMStatusSignal signal);
	void ReportModelInfo(string modelName, int32_t ctx_size, int32_t used_ctx);
	void ReportTokensPerSec(double tokPerSec);
	void ReportMemory(int64_t ram, int64_t vram, bool bIncrement);
	void ReportReadyState(ReadyState readyState);

private:
	std::timed_mutex _statusMutex; // Guards status reporting
	LLMStatus _lastStatus {};
	string _modelName;
	int32_t _ctx_size {};
	int32_t _used_ctx {};

	std::queue<LLMStatusSignal> _statusSignals;
	std::atomic<double> _tokensPerSec {};
	std::atomic<ReadyState> _readyState { ReadyState::Uninitialized };
	std::atomic<std::pair<int64_t, int64_t>> _usedRAMVRAM { {0, 0} };
};

#endif