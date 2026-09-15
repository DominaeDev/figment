#pragma once

#include "Figment.h"

namespace fig::data
{
	struct CharacterTrait
	{
		enum class Visibility
		{
			Public = 0,
			Private,
		};

		fig::handle id;
		fig::string name;
		fig::string text;
		Visibility visibility {};

		static const std::map<Visibility, fig::string> VisibilityMapping;

	public:
		static auto XmlFields() noexcept
		{
			return Fields(
				Attribute { "id", &CharacterTrait::id },
				Attribute { "visibility", &CharacterTrait::visibility,
					[](auto& value) { return enum_serialize(value, VisibilityMapping); },
					[](auto& value) { return enum_deserialize(value, VisibilityMapping); }
				},
				Element { "Name", &CharacterTrait::name },
				Element { "Value", &CharacterTrait::text }
			);

			static_assert(IsXmlSerializable<CharacterTrait>);
		}
	};
}