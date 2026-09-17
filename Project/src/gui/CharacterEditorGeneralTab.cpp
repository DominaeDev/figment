#include <pch.h>
#include "gui/CharacterEditorGeneralTab.h"
#include "gui/TextBox.h"
#include "gui/ComboBox.h"
#include "gui/ButtonWithLabel.h"
#include "gui/ButtonWithLabelAndIcon.h"
#include "gui/ToggleWithLabel.h"
#include "gui/CharacterAttributeWidget.h"
#include "gui/AppResources.h"
#include "gui/HorizontalLine.h"
#include "gui/Menu.h"
#include "gui/GridSizer.h"
#include "data/Character.h"
#include "io/FileUtility.h"

using namespace fig::data;
using namespace fig::io;

namespace fig::gui
{
	size_t CharacterEditorGeneralTab::_nextAttributeIndex {};
	static constexpr size_t kMaxTraitCount = 10uz;
	static constexpr fig::point kTraitToggleSize { 129, 35 };

	CharacterEditorGeneralTab::CharacterEditorGeneralTab(ControlPtr pParent) : EditorTab(pParent)
	{
		SetMaxWidth(1280);

		_attributesInfo.LoadFromXml(fig::path { "resources/editor/attributes.xml"});
	}

	bool CharacterEditorGeneralTab::Initialize(CharacterEditorArgs args)
	{
		if (not (bool)args.pCharacter)
			return false;

		_pCharacter = args.pCharacter;

		auto pSizer = SetSizer<VerticalSizer>();

		CreateHeader(this, pSizer, "General information");

		// Name(s)
		auto pNameSizer = new HorizontalSizer();
		auto pNameColumn1 = new VerticalSizer();
		auto pNameColumn2 = new VerticalSizer();
		pNameSizer->Add(pNameColumn1, 0, SizerFlag::FixedSize, 320);
		pNameSizer->Add(pNameColumn2, -1);
		CreateLabel(this, pNameColumn1, "First name");
		CreateTextBox(this, pNameColumn1, ValueBinding<fig::string>(&_pCharacter->name.first))
			->SetMaxWidth(300);

		CreateLabel(this, pNameColumn2, "Last name");
		CreateTextBox(this, pNameColumn2, ValueBinding<fig::string>(&_pCharacter->name.last))
			->SetMaxWidth(300);

		CreateLabel(this, pNameColumn1, "Nickname");
		CreateTextBox(this, pNameColumn1, ValueBinding<fig::string>(&_pCharacter->name.nickname))
			->SetMaxWidth(300);

		pSizer->Add(pNameSizer, 0, SizerFlag::FixedSize, 126);

		// Gender / Pronouns
		auto pGenderSizer = new HorizontalSizer();
		auto pGenderColumn1 = new VerticalSizer();
		auto pGenderColumn2 = new VerticalSizer();
		pGenderSizer->Add(pGenderColumn1, 0, SizerFlag::FixedSize, 320);
		pGenderSizer->Add(pGenderColumn2, -1);

		std::vector<fig::string> genders { "Male", "Female", "Non-binary" };
		std::vector<fig::string> pronouns { "Auto", "He/Him", "She/Her", "They/Them", "It/It" };
		CreateLabel(this, pGenderColumn1, "Gender/Sex");
		CreateComboBox(this, pGenderColumn1, genders, ValueBinding<Gender>(&_pCharacter->gender))
			->SetMaxWidth(300);

		CreateLabel(this, pGenderColumn2, "Pronouns");
		CreateDropList(this, pGenderColumn2, pronouns, ValueBinding<Pronouns>(&_pCharacter->pronouns))
			->SetMaxWidth(180);

		pSizer->Add(pGenderSizer, 0, SizerFlag::FixedSize, 63);

		// Age
		CreateLabel(this, pSizer, "Age");
		_pAge = CreateTextBox(this, pSizer);
		_pAge->SetText(_pCharacter->GetAttribute("age").value_or(""));
		_pAge->SetMaxWidth(120);

		CreateHorizontalLine(this, pSizer);

		// Attributes
		CreateHeader(this, pSizer, "Attributes");

		_pAttributeSizer = new VerticalSizer();
		pSizer->Add(_pAttributeSizer.get(), 0, SizerFlag::Expand | SizerFlag::Bottom, 8);

		for (auto& attribute : _pCharacter->GetAttributes())
		{
			if (attribute.id == "age")
				continue; // Skip

			AddAttribute(attribute);
		}

		pSizer->AddSpacer(6);

		auto pAddAttributeButton = CreateControl<ButtonWithLabel>("Add attribute");
		pAddAttributeButton->SetHeight(35);
		pAddAttributeButton->SetDelegate([this] { ShowAttributesMenu(); });
		pSizer->Add(pAddAttributeButton, 0);
		return true;
	}

	void CharacterEditorGeneralTab::OnAfterLayout()
	{
		ResizeToFit(false, true);
	}
		
	void CharacterEditorGeneralTab::ShowAttributesMenu()
	{
		auto usedAttributeIds = _items
			| std::views::transform([](auto&& i) { return i.attribute.id; })
			| std::ranges::to<std::unordered_set>();

		auto& menu = CreateMenu();
		menu.AddItem("New attribute")
			.SetDelegate([this]() { AddAttribute(); });

		if (not _attributesInfo.groups.empty())
		{
			menu.AddSeparator();

			for (auto& group : _attributesInfo.groups)
			{
				auto& groupItem = menu.AddItem(group.name);

				for (auto& attribute : group.attributes)
				{
					auto& attributeItem = groupItem.AddItem(attribute.name);
					attributeItem.SetDelegate([this, attribute]() { AddAttribute(attribute); });
					attributeItem.SetEnabled(not usedAttributeIds.contains(attribute.id));

					if (attribute.bBreak)
						groupItem.AddSeparator();
				}
			}
		}

		menu.Show();
	}

	static bool IsShiftDown()
	{
		auto mod = SDL_GetModState();
		return (mod & SDL_KMOD_SHIFT) != 0 and (mod & SDL_KMOD_CTRL) == 0 and (mod & SDL_KMOD_ALT) == 0;
	}

	fig::observer_ptr<CharacterAttributeWidget> CharacterEditorGeneralTab::AppendAttributeControl(fig::data::CharacterAttribute& attribute, size_t index, const fig::string_list& options, fig::string_view placeholder)
	{
		auto pAttribute = CreateControl<CharacterAttributeWidget>(attribute.name, attribute.value, attribute.type, options, placeholder);
		pAttribute->SetButtonDelegate([this, index] { OnAttributeSettingsMenu(index); });
		pAttribute->EnableRename(true);
		pAttribute->SetEditNameDelegate([this, index](auto&& name) { OnRenamedAttribute(index, name); });
		pAttribute->SetMoveDelegate([this, index](auto&& dir) { OnMoveAttribute(index, dir, IsShiftDown()); });
		_pAttributeSizer->Add(pAttribute, 0, SizerFlag::Expand | SizerFlag::Bottom, 8);
		return pAttribute;
	}

	fig::observer_ptr<CharacterAttributeWidget> CharacterEditorGeneralTab::AddAttribute(const fig::data::CharacterAttributeInfo& info)
	{
		auto item = AttributeItem {
			.index = _nextAttributeIndex++,
			.attribute = CharacterAttribute {
				.id = info.id,
				.name = info.name,
				.value = info.value,
				.type = info.type,
				.visibility = info.visibility,
				.flags = info.flags,
			},
		};
	
		item.pControl = AppendAttributeControl(item.attribute, item.index, info.options, info.placeholder);
		item.pControl->Focus();
		_items.push_back(std::move(item));
		return item.pControl;
	}

	fig::observer_ptr<CharacterAttributeWidget> CharacterEditorGeneralTab::AddAttribute(const CharacterAttribute& attribute)
	{
		auto item = AttributeItem {
			.index = _nextAttributeIndex++,
			.attribute = attribute,
		};
		
		// Is known attribute?
		fig::string_list options;
		fig::string_view placeholder;

		for (auto& group : _attributesInfo.groups)
		{
			for (auto& info : group.attributes)
			{
				if (info.id == item.attribute.id and item.attribute.type == CharacterAttribute::ValueType::Options)
				{
					options = info.options;
					placeholder = info.placeholder;
					break;
				}
			}
		}

		item.pControl = AppendAttributeControl(item.attribute, item.index, options, placeholder);
		_items.push_back(std::move(item));
		return item.pControl;
	}

	fig::observer_ptr<CharacterAttributeWidget> CharacterEditorGeneralTab::AddAttribute()
	{
		auto item = AttributeItem {
			.index = _nextAttributeIndex++,
			.attribute = CharacterAttribute {
				.name = "New attribute",
			},
		};

		item.pControl = AppendAttributeControl(item.attribute, item.index, {}, {});
		item.pControl->BeginEditName();
		_items.push_back(std::move(item));
		return item.pControl;
	}

	void CharacterEditorGeneralTab::OnAttributeSettingsMenu(size_t attributeIndex)
	{
		AttributeItem* pItem = nullptr;
		size_t index {};
		if (auto itFind = std::ranges::find(_items, attributeIndex, [](auto&& a) { return a.index; }); itFind != std::ranges::cend(_items))
		{
			pItem = &(*itFind);
			index = std::distance(_items.begin(), itFind);
		}
		else
			return;

		auto pAttribute = &pItem->attribute;
		auto pControl = pItem->pControl;

		auto& menu = CreateMenu();
		auto& typeMenu = menu.AddItem("Value type");
		typeMenu.AddCheckItem("Text (single line)", pAttribute->type == CharacterAttribute::ValueType::ShortText)
			.SetDelegate([pAttribute, pControl] {
				pAttribute->type = CharacterAttribute::ValueType::ShortText;
				pControl->ChangeType(pAttribute->type);
			});
		typeMenu.AddCheckItem("Text (multiple lines)", pAttribute->type == CharacterAttribute::ValueType::LongText)
			.SetDelegate([pAttribute, pControl] {
				pAttribute->type = CharacterAttribute::ValueType::LongText;
				pControl->ChangeType(pAttribute->type);
			});
		typeMenu.AddCheckItem("Number", pAttribute->type == CharacterAttribute::ValueType::Number)
			.SetDelegate([pAttribute, pControl] {
				pAttribute->type = CharacterAttribute::ValueType::Number;
				pControl->ChangeType(pAttribute->type);
			});
		typeMenu.AddCheckItem("Comma-separated list", pAttribute->type == CharacterAttribute::ValueType::List)
			.SetDelegate([pAttribute, pControl] {
				pAttribute->type = CharacterAttribute::ValueType::List;
				pControl->ChangeType(pAttribute->type);
			});

		auto& visibilityMenu = menu.AddItem("Visibility");
		visibilityMenu.AddCheckItem("Public", pAttribute->visibility == CharacterAttribute::Visibility::Public)
			.SetDelegate([pAttribute, pControl] {
				pAttribute->visibility = CharacterAttribute::Visibility::Public;
			});
		visibilityMenu.AddCheckItem("Private", pAttribute->visibility == CharacterAttribute::Visibility::Private)
			.SetDelegate([pAttribute, pControl] {
				pAttribute->visibility = CharacterAttribute::Visibility::Private;
			});

		auto& priorityMenu = menu.AddItem("Priority");
		priorityMenu.AddCheckItem("Trivial", pAttribute->flags.IsSet(CharacterAttribute::HintFlag::Trivial))
			.SetDelegate([pAttribute, pControl] {
				pAttribute->flags.Flip(CharacterAttribute::HintFlag::Trivial); 
				pAttribute->flags.Unset(CharacterAttribute::HintFlag::Important);
			});
		priorityMenu.AddCheckItem("Important", pAttribute->flags.IsSet(CharacterAttribute::HintFlag::Important))
			.SetDelegate([pAttribute, pControl] {
				pAttribute->flags.Flip(CharacterAttribute::HintFlag::Important); 
				pAttribute->flags.Unset(CharacterAttribute::HintFlag::Trivial);
			});

		menu.AddSeparator();
		menu.AddItem("Rename\u2026", Resource::ICON_EDIT)
			.SetDelegate([this, attributeIndex] { RenameAttribute(attributeIndex); });
		auto& moveMenu = menu.AddItem("Move");
		if (_items.size() > 1uz)
		{
			moveMenu.AddItem("Move to top")
				.SetEnabled(index > 0uz)
				.SetDelegate([this, attributeIndex] { OnMoveAttribute(attributeIndex, -1, true); });
			moveMenu.AddItem("Move up")
				.SetEnabled(index > 0uz)
				.SetDelegate([this, attributeIndex] { OnMoveAttribute(attributeIndex, -1, false); });
			moveMenu.AddItem("Move down")
				.SetEnabled(index + 1uz < _items.size())
				.SetDelegate([this, attributeIndex] { OnMoveAttribute(attributeIndex, 1, false); });
			moveMenu.AddItem("Move to bottom")
				.SetEnabled(index + 1uz < _items.size())
				.SetDelegate([this, attributeIndex] { OnMoveAttribute(attributeIndex, 1, true); });
		}
		else
		{
			moveMenu.SetEnabled(false);
		}

		menu.AddSeparator();
		menu.AddItem("Copy");
		menu.AddItem("Paste");
		menu.AddSeparator();
		menu.AddItem("Remove", Resource::ICON_DELETE)
			.SetDelegate([this, attributeIndex] { RemoveAttribute(attributeIndex); });

		menu.Show();
	}

	void CharacterEditorGeneralTab::RenameAttribute(size_t index)
	{
		if (auto itFind = std::ranges::find(_items, index, [](auto&& a) { return a.index; }); itFind != std::ranges::cend(_items))
		{
			auto& item = *itFind;
			item.pControl->BeginEditName();
		}
	}

	void CharacterEditorGeneralTab::OnRenamedAttribute(size_t index, fig::string_view name)
	{
		if (auto itFind = std::ranges::find(_items, index, [](auto&& a) { return a.index; }); itFind != std::ranges::cend(_items))
		{
			auto& item = *itFind;
			item.attribute.name = name;
			item.attribute.id = fig::handle { name };
			item.pControl->Focus();
		}
	}

	void CharacterEditorGeneralTab::RemoveAttribute(size_t index)
	{
		if (auto itFind = std::ranges::find(_items, index, [](auto&& a) { return a.index; }); itFind != std::ranges::cend(_items))
		{
			auto& item = *itFind;
			_pAttributeSizer->Remove(item.pControl);
			DestroyChild(item.pControl);
			_items.erase(itFind);
		}
	}

	void CharacterEditorGeneralTab::OnMoveAttribute(size_t attributeIndex, int32_t dir, bool bMaxDistance)
	{
		if (auto itFind = std::ranges::find(_items, attributeIndex, [](auto&& a) { return a.index; }); itFind != std::ranges::cend(_items))
		{
			size_t index = std::distance(std::ranges::begin(_items), itFind);
			if (dir < 0 and bMaxDistance) // Move to top
			{
				if (index == 0uz)
					return;
				AttributeItem item = *itFind;
				_items.erase(itFind);
				_items.insert(_items.cbegin(), item);
			}
			else if (dir < 0) // Move up
			{
				if (index == 0uz)
					return;
				std::swap(_items[index - 1], _items[index]);
			}
			if (dir > 0 and bMaxDistance) // Move to bottom
			{
				if (index + 1 >= _items.size())
					return;
				
				AttributeItem item = *itFind;
				_items.erase(itFind);
				_items.insert(_items.cbegin() + _items.size(), item);
			}
			else if (dir > 0) // Move down
			{
				if (index + 1 >= _items.size())
					return;
				std::swap(_items[index], _items[index + 1]);
			}
			
			_pAttributeSizer->RemoveAll();
			for (auto& item : _items)
				_pAttributeSizer->Add(item.pControl, 0, SizerFlag::Expand | SizerFlag::Bottom, 8);
			InvalidateLayout();
		}
	}

	EditorTabBase::SaveResult CharacterEditorGeneralTab::OnSave() noexcept
	{
		// Ensure unique ids
		std::map<fig::handle, std::vector<CharacterAttribute*>> attributesById;
		for (auto& item : _items)
		{
			auto& attribute = item.attribute;
			if (attribute.id.empty())
				attribute.id = fig::handle { attribute.name };
			
			attributesById[attribute.id].push_back(&attribute);
		}

		for (auto& kvp : attributesById)
		{
			if (kvp.second.size() > 1)
			{
				auto prefix = (fig::string)kvp.first;
				auto& attributes = kvp.second;
				for (size_t i = 1uz; i < attributes.size(); ++i)
					attributes[i]->id = fig::handle { std::format("{}{}", prefix, i + 1) };
			}
		}

		// Write attributes
		_pCharacter->ClearAttributes();
		
		// Write age
		if (auto age = _pAge->GetText(); not age.empty())
			_pCharacter->SetAttribute("age", "Age", age);

		for (auto& item : _items)
		{
			auto& attribute = item.attribute;
			fig::string_view value = item.pControl->GetValue();

			_pCharacter->SetAttribute(attribute.id, attribute.name, value, attribute.type, attribute.visibility, attribute.flags);
		}

		return {};
	}

}