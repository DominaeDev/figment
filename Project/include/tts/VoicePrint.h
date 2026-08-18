#pragma once

#include "Figment.h"
#include "tts/TTSTypes.h"

namespace fig::tts
{
	struct VoicePrint
	{
		fig::uuid modelId;
		fig::string generationPrompt;
		fig::string referenceText;
		std::vector<fig::string> keys;
		uint32_t seed;

		AudioData audioData;

		static auto XmlFields() noexcept
		{
			using namespace fig::data;
			return Fields(
				Element { "Model",			&VoicePrint::modelId },
				Element { "Prompt",			&VoicePrint::generationPrompt },
				Element { "Seed",			&VoicePrint::seed },
				Element { "Transcript",		&VoicePrint::referenceText },
				Element { "Keys",			&VoicePrint::keys },
				Element { "Data",			&VoicePrint::audioData,
					[](auto&& data) { return data.AsBase64(); },
					[](auto&& data) { return AudioData::FromBase64(data); }
				}
					
			);
			static_assert(IsXmlSerializable<VoicePrint>);
		}
	};
}