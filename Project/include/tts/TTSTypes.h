#pragma once

#include "Figment.h"
#include <future>

namespace fig::tts
{
	enum class TTSTask
	{
		Undefined = 0,
		Speak,
		Design,
	};

	constexpr auto TTSTaskMapping = std::array<std::pair<TTSTask, std::string_view>, 2> {
		std::pair { TTSTask::Speak,		"tts" },
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

	using TTSData = fig::bytes;
	using TTSPayload = std::expected<TTSData, TTSError>;
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