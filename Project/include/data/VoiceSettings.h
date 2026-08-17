#pragma once

#include "data/CharacterGender.h"
#include "io/XmlData.h"
#include "tts/VoicePrint.h"

namespace fig::data
{
	struct VoiceSettings : public XmlData<"VoiceSettings", 0>
	{
		fig::string name; //??
		fig::string description; //??
		fig::string language; //! @maybe
		fig::uuid modelId; //! @maybe
		fig::tts::VoicePrint voicePrint;

		static auto XmlFields() noexcept
		{
			return Fields(
				Element { "Name",			&VoiceSettings::name },
				Element { "Description",	&VoiceSettings::description },
				Element { "Model",			&VoiceSettings::modelId },
				Element { "Language",		&VoiceSettings::language },
				Element { "Voice",			&VoiceSettings::voicePrint}
			);

			static_assert(IsXmlSerializable<VoiceSettings>);
		}
	};
}
