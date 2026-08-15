#pragma once

#include "Figment.h"
#include "tts/TTSTypes.h"

namespace fig::tts
{
	struct VoicePrint
	{
		TTSData audioData;
		fig::string prompt;
		fig::uuid modelId;
		std::unordered_set<fig::handle> keys;
	};
}