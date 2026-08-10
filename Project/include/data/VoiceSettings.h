#pragma once

#include "data/CharacterGender.h"
#include "io/XmlData.h"

namespace fig::data
{
	struct VoiceSettings : public XmlData<"VoiceSettings", 0>
	{
		fig::string name;
		fig::string description;
		Gender gender;
		fig::uuid modelId;
		fig::string language;
		fig::string referenceText;
		std::vector<fig::handle> tags;

		static auto XmlFields() noexcept
		{
			return Fields(
				Element { "Name",			&VoiceSettings::name },
				Element { "Description",	&VoiceSettings::description },
				Element { "Gender",			&VoiceSettings::gender,
					[](auto& value) { return (fig::string)value; },
					[](auto& value) { return Gender(value); }
				},
				Element { "Model",			&VoiceSettings::modelId },
				Element { "Language",		&VoiceSettings::language },
				Element { "Reference",		&VoiceSettings::referenceText },
				Element { "Tags",			&VoiceSettings::tags }
			);

			static_assert(IsXmlSerializable<VoiceSettings>);
		}
	};
}
