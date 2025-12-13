#ifndef STATUS_SIGNAL_H__
#define STATUS_SIGNAL_H__
#pragma once

#include "Types.h"
#include <queue>
#include <mutex>
#include <optional>

enum class LLMStatusSignal {
	Nothing = 0,
	LoadingModel,
	LoadedModel,
	LoadModelFailure,
	UnloadedModel,
	GenerationStarted,
	GenerationComplete,
	InitializingChat,
	InitializedChat,
	InitializeChatFailure,
	CompletedMessage,
	RebuildingContext,
};

enum class ReadyState 
{ 
	Invalid,		// Invalid state -> Trigger unload
	LoadError,
	Uninitialized, 
	LoadingModel,
	ModelLoaded, 
	Initializing, 
	Ready, 
	Generating, 
	RebuildingContext,
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
	inline bool IsInvalid() const noexcept { return readyState == ReadyState::Invalid; }
};

class LLMStatusChannel
{
public:
	LLMStatus PollStatus();
	void PushSignal(LLMStatusSignal signal);

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