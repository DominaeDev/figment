#pragma once

#include "data/CharacterAttribute.h"
#include "io/XmlData.h"

namespace fig::data
{
	struct CharacterAttributeInfo
	{
		fig::handle id;
		fig::string name;
		CharacterAttribute::ValueType type {};
		CharacterAttribute::Visibility visibility {};
		CharacterAttribute::HintFlags flags {};
		fig::string_list options;
		fig::string placeholder;
		fig::string value;
		bool bBreak;

		static auto XmlFields() noexcept
		{
			return Fields(
				Attribute { "id", &CharacterAttributeInfo::id }
					.MustExist(),
				Attribute { "type", &CharacterAttributeInfo::type,
					[](auto&& value) { return enum_serialize(value, CharacterAttribute::ValueTypeMapping); },
					[](auto&& value) { return enum_deserialize(value, CharacterAttribute::ValueTypeMapping); }
				},
				Attribute { "visibility", &CharacterAttributeInfo::visibility,
					[](auto&& value) { return enum_serialize(value, CharacterAttribute::VisibilityMapping); },
					[](auto&& value) { return enum_deserialize(value, CharacterAttribute::VisibilityMapping); }
				},
				Attribute { "flags", &CharacterAttributeInfo::flags,
					[](auto&& value) { return enum_serialize_flags(value, CharacterAttribute::FlagMapping); },
					[](auto&& value) { return enum_deserialize_flags(value, CharacterAttribute::FlagMapping); }
				},
				Attribute { "break", &CharacterAttributeInfo::bBreak },

				Element { "Name", &CharacterAttributeInfo::name },
				Element { "Value", &CharacterAttributeInfo::value },
				Element { "Placeholder", &CharacterAttributeInfo::placeholder },
				Element { "Options", &CharacterAttributeInfo::options,
					[](auto&& value) { return encode_csv(value); },
					[](auto&& value) { return decode_csv(value); }
				}
			);

			static_assert(IsXmlSerializable<CharacterAttributeInfo>);
		}
	};

	struct CharacterAttributeInfoDatabase : XmlData<"Attributes">
	{
		struct AttributeGroup
		{
			fig::string name;
			std::vector<CharacterAttributeInfo> attributes;

			static auto XmlFields() noexcept
			{
				using namespace fig::data;

				return Fields(
					Attribute { "name", &AttributeGroup::name }
						.MustExist(),
					Element { "Attribute", &AttributeGroup::attributes }
						.MustExist()
				);

				static_assert(IsXmlSerializable<CharacterAttributeInfo>);
			}
		};
		std::vector<AttributeGroup> groups;

		static auto XmlFields() noexcept
		{
			using namespace fig::data;

			return Fields(
				Element { "Group", &CharacterAttributeInfoDatabase::groups }
			);

			static_assert(IsXmlSerializable<CharacterAttributeInfoDatabase>);
		}
	};
}