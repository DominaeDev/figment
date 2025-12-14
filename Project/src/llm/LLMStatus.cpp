#include "llm/LLMStatus.h"
#include "util/Lockable.h"

using namespace fig::llm;

LLMStatus LLMStatusChannel::PollStatus()
{
	LLMStatus status {};

	// Try to acquire state lock
	std::unique_lock<std::timed_mutex> lock(_statusMutex, std::try_to_lock);
	if (lock.owns_lock())
	{
		status.modelName = _modelName;
		status.allocCtxSize = _ctx_size;
		status.usedCtxSize = _used_ctx;
		if (!_statusSignals.empty())
		{
			status.signal = _statusSignals.front();
			_statusSignals.pop();
		};
		lock.unlock();
	}

	status.readyState = _readyState.load();
	status.tokensPerSec = _tokensPerSec.load();
	auto memory = _usedRAMVRAM.load();
	status.usedRAM = memory.first;
	status.usedVRAM = memory.second;

	return status;
}

void LLMStatusChannel::EmitSignal(LLMStatusSignal signal)
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
	_tokensPerSec.store(tokPerSec);
}

void LLMStatusChannel::ReportReadyState(ReadyState readyState)
{
	_readyState.store(readyState);
}

void LLMStatusChannel::ReportMemory(int64_t ram, int64_t vram, bool bIncrement)
{
	if (bIncrement)
	{
		auto mem = _usedRAMVRAM.load();
		mem.first += ram;
		mem.second += vram;
		_usedRAMVRAM.store(mem);
	}
	else
		_usedRAMVRAM.store({ram, vram});
}