#pragma once

#include "data/CharacterTrait.h"
#include "io/XmlData.h"

namespace fig::data
{
	struct CharacterTraitInfo
	{
		fig::handle id;
		fig::string name;
		fig::string text;
		CharacterTrait::Visibility visibility {};

		static auto XmlFields() noexcept
		{
			return Fields(
				Attribute { "id", &CharacterTraitInfo::id }
					.MustExist(),
				Attribute { "visibility", &CharacterTraitInfo::visibility,
					[](auto&& value) { return enum_serialize(value, CharacterTrait::VisibilityMapping); },
					[](auto&& value) { return enum_deserialize(value, CharacterTrait::VisibilityMapping); }
				},
				Element { "Name", &CharacterTraitInfo::name },
				Element { "Description", &CharacterTraitInfo::text }
			);

			static_assert(IsXmlSerializable<CharacterTraitInfo>);
		}
	};

	struct CharacterTraitInfoDatabase : XmlData<"Traits">
	{
		struct TraitGroup
		{
			fig::string name;
			std::vector<CharacterTraitInfo> traits;

			static auto XmlFields() noexcept
			{
				using namespace fig::data;

				return Fields(
					Attribute { "name", &TraitGroup::name }
						.MustExist(),
					Element { "Trait", &TraitGroup::traits }
						.MustExist()
				);

				static_assert(IsXmlSerializable<CharacterTraitInfo>);
			}
		};
		std::vector<TraitGroup> groups;

		static auto XmlFields() noexcept
		{
			using namespace fig::data;

			return Fields(
				Element { "Group", &CharacterTraitInfoDatabase::groups }
			);

			static_assert(IsXmlSerializable<CharacterTraitInfoDatabase>);
		}
	};
}