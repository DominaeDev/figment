#pragma once

#include "Figment.h"
#include "tts/AudioData.h"
#include <future>

namespace fig::tts
{
	enum class TTSTaskType
	{
		Undefined = 0,
		Speech,
		Design,
		Unload,
	};

	constexpr auto TTSTaskMapping = std::array<std::pair<TTSTaskType, std::string_view>, 2> {
		std::pair { TTSTaskType::Speech,	"tts" },
		std::pair { TTSTaskType::Design,	"design" },
	};

	enum class TTSError
	{
		NoError = 0,
		Failed,
		Canceled,
		Busy,			// Server responded with 503 busy
		Unavailable,	// Server didn't respond
	};


	struct TTSPayload
	{
		AudioData audioData;
		fig::uuid sourceId;
	};
	
	using TTSResult = std::expected<TTSPayload, TTSError>;
	using TTSPromise = std::promise<TTSResult>;
	using TTSFuture = std::future<TTSResult>;

	struct TTSTask
	{
		uint64_t id;
		TTSTaskType task;
		TTSFuture future;
	};

	enum TTSStatus
	{
		Uninitialized,
		ServerStarted,
		ServerBusy,
	};
}