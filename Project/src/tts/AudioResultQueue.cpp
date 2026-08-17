#include <pch.h>
#include "tts/AudioResultQueue.h"
#include "audio/AudioManager.h"

namespace fig::tts
{
	void AudioResultQueue::Update()
	{
		// Resolve pending results
		if (not _pendingResults.empty())
		{
			for (auto& result : _pendingResults)
			{
				auto& future = result.future;
				if (not future.valid())
					continue;

				if (future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
				{
					if (auto payload = future.get())
					{
						if (_fnDelegate)
							_fnDelegate(std::move(payload).value());
					}
					else
					{
						if (_fnDelegate)
							_fnDelegate(std::unexpected(payload.error()));
					}
				}
			}

			std::erase_if(_pendingResults, [](auto&& r) { return not r.future.valid(); });
		}
	}

	void AudioResultQueue::Add(fig::tts::TTSResult&& result)
	{
		_pendingResults.emplace_back(std::move(result));
	}

	void AudioResultQueue::Clear()
	{
		_pendingResults.clear();
	}
}