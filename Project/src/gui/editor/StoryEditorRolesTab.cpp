#include <pch.h>
#include "gui/editor/StoryEditorRolesTab.h"
#include "gui/editor/StoryRoleEditWidget.h"
#include "gui/editor/StoryRoleSlotBox.h"
#include "gui/ButtonWithIcon.h"
#include "gui/Menu.h"
#include "gui/AppResources.h"
#include "data/Scenario.h"
#include "data/Character.h"
#include "util/StringUtils.h"

using namespace fig::chat;
using namespace fig::data;

namespace fig::gui
{
	constexpr size_t MaxRoles = MaxBots() + 1;

	StoryEditorRolesTab::StoryEditorRolesTab(ControlPtr pParent) : EditorTab(pParent)
	{
		SetMaxWidth(1280);
	}

	bool StoryEditorRolesTab::OnInit(StoryEditorArgs args)
	{
		if (not (bool)args.pScenario)
			return false;

		_pScenario = args.pScenario;
		_pCharacter = args.pCharacter;

		auto pSizer = SetSizer<VerticalSizer>();
		auto pSlotSizer = new HorizontalSizer();

		CreateHeader(this, pSizer, "Roles");

		pSizer->Add(pSlotSizer, 0);

		CreateHorizontalLine(this, pSizer);

		constexpr std::array<Role, MaxRoles> roles = { Role::Bot1, Role::Bot2, Role::Bot3, Role::Bot4, Role::User };

		for (auto role : roles)
			CreateRole(pSlotSizer, pSizer, role);

		_pAddButton = CreateControl<ButtonWithIcon>(Icon::PLUS_SMALL, true);
		_pAddButton->SetSize(114, 152);
		_pAddButton->SetDelegate([this] { AddRole(); });
		pSlotSizer->Add(_pAddButton, 0);

		return true;
	}

	EditorTabBase::SaveResult StoryEditorRolesTab::OnSave() noexcept
	{
		return {};
	}

	void StoryEditorRolesTab::CreateRole(SizerPtr pBoxSizer, SizerPtr pEditSizer, Role role)
	{
		bool bMulti = _pScenario->GetRoleSlots().size() > 2uz;

		// Header
		auto pHeader = CreateHeader(this, pEditSizer, "");

		// Box
		auto pBox = CreateControl<StoryRoleSlotBox>();
		pBox->SetRole(role, "");
		pBox->SetRightClickDelegate([this, role] { ShowRoleSlotMenu(role); });
		pBoxSizer->Add(pBox, 0, SizerFlag::Right, 12);

		// Edit
		auto pEdit = CreateControl<StoryRoleEditWidget>(_pScenario.get(), _pCharacter.get(), role);
		pEdit->SetLocked(IsCharacterStory());
		pEdit->SetNameChangedDelegate([this, role](auto&& name) {
			if (auto itItem = std::ranges::find(_roleItems, role, [](auto&& i) { return i.role; }); itItem != std::ranges::cend(_roleItems))
				(*itItem).pBox->SetRole(role, name);
		});
		pEditSizer->Add(pEdit, 0, SizerFlag::Expand | SizerFlag::Bottom, 8);
		
		_roleItems.emplace_back(RoleItem {
			.role = role,
			.pHeader = pHeader,
			.pBox = pBox,
			.pEdit = pEdit,
		});
	}

	void StoryEditorRolesTab::OnTabSelected()
	{
		RefreshRoles();
	}

	void StoryEditorRolesTab::RefreshRoles()
	{
		auto& roles = _pScenario->GetRoleSlots();

		for (size_t i = 0; i < _roleItems.size(); ++i)
		{
			auto& item = _roleItems[i];
			bool bActive = false;
			if (auto roleSlot = _pScenario->GetRoleSlot(item.role))
			{
				bActive = true;
				item.pBox->SetRole(item.role, (*roleSlot).label);
				item.pEdit->Refresh();
				item.pEdit->SetLocked(IsCharacterStory());

				if (is_bot(item.role))
				{
					if (_pCharacter)
					{
						item.pBox->SetRole(item.role, _pCharacter->name.GetSpokenName());
						item.pHeader->SetText(std::format("{}'s role", _pCharacter->name.GetSpokenName()));
					}
					else
					{
						if (roles.size() > 2uz)
							item.pHeader->SetText(std::format("AI role #{}", static_cast<int32_t>(item.role)));
						else
							item.pHeader->SetText("AI role");
					}
				}
				else
					item.pHeader->SetText("User's role");
			}

			item.pHeader->SetVisible(bActive);
			item.pHeader->EnableLayout(bActive);
			item.pBox->SetVisible(bActive);
			item.pBox->EnableLayout(bActive);
			item.pEdit->SetVisible(bActive);
			item.pEdit->EnableLayout(bActive);
		}
		InvalidateLayout();

		_pAddButton->SetVisible(roles.size() < MaxRoles);
		_pAddButton->SetEnabled(not IsCharacterStory());
	}

	void StoryEditorRolesTab::AddRole()
	{
		for (uint8_t i = 0; i < MaxBots(); ++i)
		{
			Role role = bot_from_index(i);
			if (not _pScenario->GetRoleSlot(role).has_value())
			{
				_pScenario->AppendRoleSlot(role, std::format("Role #{}", _pScenario->roles.size()));
				RefreshRoles();
				break;
			}
		}
	}

	void StoryEditorRolesTab::ShowRoleSlotMenu(fig::chat::Role role)
	{
		auto& menu = CreateMenu();
		auto& swapItem = menu.AddItem("Swap with user");
		swapItem.SetEnabled(is_bot(role) and not IsCharacterStory());
		menu.AddSeparator();
		auto& removeItem = menu.AddItem("Remove role");
		removeItem.SetEnabled(is_bot(role) and not IsCharacterStory() and _pScenario->GetRoleSlots().size() > 2uz);
		removeItem.SetDelegate([this, role] { 
			if (_pScenario->RemoveRoleSlot(role))
				RefreshRoles();
		});

		menu.Show();
	}

	bool StoryEditorRolesTab::IsCharacterStory() const noexcept
	{
		return (bool)_pCharacter;
	}
}