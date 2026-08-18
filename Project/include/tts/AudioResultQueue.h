#pragma once

#include "Figment.h"
#include "tts/TTSTypes.h"

namespace fig::tts
{
	using AudioResultCallback = std::function<void(TTSPayload&&)>;

	class AudioResultQueue
	{
	public:
		void SetDelegate(AudioResultCallback fnDelegate) { _fnDelegate = fnDelegate; }
		void Update();
		void Add(fig::tts::TTSResult&& result);
		void Clear();
		bool IsEmpty() const noexcept { return _pendingResults.empty(); }

	private:
		std::vector<fig::tts::TTSResult> _pendingResults;
		AudioResultCallback _fnDelegate;
	};
}

