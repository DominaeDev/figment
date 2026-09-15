#pragma once

#include "gui/EditorTab.h"
#include "gui/CharacterEditorArgs.h"
#include "data/CharacterAttribute.h"
#include "io/XmlData.h"

namespace fig::data
{
	class Character;
}

namespace fig::gui
{
	class CharacterAttributeWidget;
	class TextBox;

	class CharacterEditorGeneralTab : public EditorTab<CharacterEditorArgs>
	{
	public:
		CharacterEditorGeneralTab(ControlPtr pParent);
		
		bool Initialize(CharacterEditorArgs args) override;
		SaveResult OnSave() noexcept override;
		void ShutDown() noexcept {};

	private:
		struct AttributeInfo
		{
			fig::handle id;
			fig::string name;
			fig::data::CharacterAttribute::ValueType type {};
			fig::data::CharacterAttribute::Visibility visibility {};
			fig::data::CharacterAttribute::HintFlags flags {};
			fig::string_list options;
			fig::string placeholder;
			fig::string value;
			bool bBreak;

			static auto XmlFields() noexcept
			{
				using namespace fig::data;

				return Fields(
					Attribute { "id", &AttributeInfo::id }
						.MustExist(),
					Attribute { "type", &AttributeInfo::type,
						[](auto&& value) { return enum_serialize(value, CharacterAttribute::FormatMapping); },
						[](auto&& value) { return enum_deserialize(value, CharacterAttribute::FormatMapping); }
					},
					Attribute { "visibility", &AttributeInfo::visibility,
						[](auto&& value) { return enum_serialize(value, CharacterAttribute::VisibilityMapping); },
						[](auto&& value) { return enum_deserialize(value, CharacterAttribute::VisibilityMapping); }
					},
					Attribute { "flags", &AttributeInfo::flags,
						[](auto&& value) { return enum_serialize_flags(value, CharacterAttribute::FlagMapping); },
						[](auto&& value) { return enum_deserialize_flags(value, CharacterAttribute::FlagMapping); }
					},
					Attribute { "break", &AttributeInfo::bBreak },

					Element { "Name", &AttributeInfo::name },
					Element { "Value", &AttributeInfo::value },
					Element { "Placeholder", &AttributeInfo::placeholder },
					Element { "Options", &AttributeInfo::options,
						[](auto&& value) { return encode_csv(value); },
						[](auto&& value) { return decode_csv(value); }
					}
				);

				static_assert(IsXmlSerializable<AttributeInfo>);
			}
		};

		struct AttributeGroup
		{
			fig::string name;
			std::vector<AttributeInfo> attributes;

			static auto XmlFields() noexcept
			{
				using namespace fig::data;

				return Fields(
					Attribute { "name", &AttributeGroup::name }
						.MustExist(),
					Element { "Attribute", &AttributeGroup::attributes }
						.MustExist()
				);

				static_assert(IsXmlSerializable<AttributeGroup>);
			}
		};

		struct Attributes : fig::data::XmlData<"Attributes">
		{
			std::vector<AttributeGroup> groups;

			static auto XmlFields() noexcept
			{
				using namespace fig::data;

				return Fields(
					Element { "Group", &Attributes::groups }
				);

				static_assert(IsXmlSerializable<Attributes>);
			}
		} _attributesInfo;

		fig::observer_ptr<CharacterAttributeWidget> AddAttribute();
		fig::observer_ptr<CharacterAttributeWidget> AddAttribute(const AttributeInfo& info);
		fig::observer_ptr<CharacterAttributeWidget> AddAttribute(const fig::data::CharacterAttribute& attribute);
		fig::observer_ptr<CharacterAttributeWidget> AppendAttributeControl(fig::data::CharacterAttribute& attribute, size_t index, const fig::string_list& options, fig::string_view placeholder);
		void RenameAttribute(size_t index);
		void OnRenamedAttribute(size_t index, fig::string_view name);
		void RemoveAttribute(size_t index);


	private:
		void ShowAttributesMenu();
		void OnAttributeSettingsMenu(size_t attributeIndex);
		void OnAfterLayout();

		fig::observer_ptr<fig::data::Character> _pCharacter {};

		struct AttributeItem
		{
			size_t index;
			fig::observer_ptr<CharacterAttributeWidget> pControl;
			fig::data::CharacterAttribute attribute;
		};

		std::vector<AttributeItem> _items;
		static size_t _nextAttributeIndex;

		fig::observer_ptr<TextBox> _pAge;
		SizerPtr _pAttributeSizer {};
	};
}