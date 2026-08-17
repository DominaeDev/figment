#pragma once

#include "tts/VoiceModelSettings.h"
#include "data/VoiceSettings.h"

namespace fig::tts
{
	class AudioServerConfiguration
	{
	public:
		enum class Backend
		{
			CPU,
			CUDA,
		};

		Backend backend { Backend::CUDA };
		VoiceModelSettings models {};
		std::vector<fig::data::VoiceSettings> voices {};

		fig::string ToJson() const noexcept;
	};
}