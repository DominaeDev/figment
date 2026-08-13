#pragma once

#include "Figment.h"

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
}