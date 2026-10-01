#pragma once

#include "tts/TTSModelSettings.h"
#include "data/VoiceSettings.h"
#include "data/VersionNumber.h"

namespace fig::tts
{
	class AudioServerConfiguration
	{
	public:
		enum class Backend
		{
			CPU,
			CUDA,
			Vulkan,
			Metal,
		};

		fig::path serverPath;
		fig::data::VersionNumber serverVersion;
		Backend backend { Backend::CUDA };
		TTSModelSettings models {};
		std::vector<fig::data::VoiceSettings> voices {};

		fig::string ToJson() const noexcept;
	};
}