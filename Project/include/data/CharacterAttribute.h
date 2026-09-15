#pragma once

#include "Figment.h"

namespace fig::data
{
	struct CharacterAttribute
	{
		enum class ValueType
		{
			ShortText = 0,
			LongText,
			Number,
			List,
		};

		enum class Visibility
		{
			Public = 0,
			Private,
		};

		enum class HintFlag
		{
			Trivial = 1 << 0,	// Can be omitted
			Important = 1 << 1,	// Mustn't be omitted
			Memory = 1 << 2,	// Can be placed in memory
		};
		using HintFlags = EnumFlags<HintFlag>;

		fig::handle id;
		fig::string name;
		fig::string value;
		ValueType type {};
		Visibility visibility {};
		HintFlags flags {};

		static const std::map<CharacterAttribute::ValueType, fig::string> FormatMapping;
		static const std::map<CharacterAttribute::Visibility, fig::string> VisibilityMapping;
		static const std::map<CharacterAttribute::HintFlag, fig::string> FlagMapping;

	public:
		static auto XmlFields() noexcept
		{
			return Fields(
				Attribute { "id", &CharacterAttribute::id },
				Attribute { "format", &CharacterAttribute::type,
					[](auto& value) { return enum_serialize(value, FormatMapping); },
					[](auto& value) { return enum_deserialize(value, FormatMapping); }
				},
				Attribute { "visibility", &CharacterAttribute::visibility,
					[](auto& value) { return enum_serialize(value, VisibilityMapping); },
					[](auto& value) { return enum_deserialize(value, VisibilityMapping); }
				},
				Attribute { "flags", &CharacterAttribute::flags,
					[](auto& value) -> fig::string { return encode_csv(CharacterAttribute::HintFlags::Serialize(value, FlagMapping)); },
					[](const fig::string& value) { return CharacterAttribute::HintFlags::Deserialize(decode_csv(value), FlagMapping); }
				},
				Element { "Label", &CharacterAttribute::name },
				Element { "Value", &CharacterAttribute::value }
			);

			static_assert(IsXmlSerializable<CharacterAttribute>);
		}
	};
}