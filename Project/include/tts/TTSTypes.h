#pragma once

#include "Figment.h"
#include "tts/AudioData.h"
#include <future>

namespace fig::tts
{
	enum class TTSTask
	{
		Undefined = 0,
		Speech,
		Design,
		Unload,
	};

	constexpr auto TTSTaskMapping = std::array<std::pair<TTSTask, std::string_view>, 2> {
		std::pair { TTSTask::Speech,		"tts" },
		std::pair { TTSTask::Design,	"design" },
	};

	enum class TTSError
	{
		NoError = 0,
		Failed,
		Canceled,
		Busy,			// Server responded with 503 busy
		Unavailable,	// Server didn't respond
	};

	
	using TTSPayload = std::expected<AudioData, TTSError>;
	using TTSPromise = std::promise<TTSPayload>;
	using TTSFuture = std::future<TTSPayload>;

	struct TTSResult
	{
		uint64_t id;
		TTSTask task;
		TTSFuture future;
	};

	enum TTSStatus
	{
		Uninitialized,
		ServerStarted,
		ServerBusy,
	};
}