#include <pch.h>
#include <chrono>
#include "llm/LLMStatus.h"
#include "util/Common.h"
#include "util/Lockable.h"

using namespace fig::llm;
using namespace fig::util;

using namespace std::chrono_literals;

std::optional<LLMStatus> LLMStatusChannel::PollStatus()
{
	// Try to acquire status lock
	std::unique_lock lock(_statusMutex, std::defer_lock);
	if (!lock.try_lock_for(50ms))
		return std::nullopt;

	LLMStatus status
	{
		.modelName = _modelName,
		.allocCtxSize = _ctx_size,
		.usedCtxSize = _used_ctx,
	};

	if (!_statusSignals.empty())
	{
		status.event = _statusSignals.front();
		_statusSignals.pop();
	};
	lock.unlock();

	status.readyState = _readyState;
	status.tokensPerSec = _tokensPerSec;
	auto& [ram, vram] = _usedRAMVRAM;
	status.usedRAM = ram;
	status.usedVRAM = vram;
	_lastStatus = status;
	return status;
}

void LLMStatusChannel::EmitSignal(LLMStatusEvent signal)
{
	std::scoped_lock _ { _statusMutex };

	if (!_statusSignals.empty() && _statusSignals.back() == signal)
		return;

	_statusSignals.push(signal);
}

void LLMStatusChannel::ReportModelInfo(fig::string modelName, int32_t ctx_size, int32_t used_ctx)
{
	std::scoped_lock _ { _statusMutex };
	_modelName = modelName;
	_ctx_size = ctx_size;
	_used_ctx = used_ctx;
}

void LLMStatusChannel::ReportTokensPerSec(double tokPerSec)
{
	std::scoped_lock _ { _statusMutex };
	_tokensPerSec = tokPerSec;
}

void LLMStatusChannel::ReportReadyState(ReadyState readyState)
{
	std::scoped_lock _ { _statusMutex };
	_readyState = readyState;
}

void LLMStatusChannel::ReportMemory(int64_t ram, int64_t vram, bool bIncrement)
{
	if (bIncrement)
	{
		_usedRAMVRAM.first += ram;
		_usedRAMVRAM.second += vram;
	}
	else
		_usedRAMVRAM = {ram, vram};
}