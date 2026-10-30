#include <pch.h>
#include "gui/StoryEditorRolesTab.h"
#include "gui/TextBox.h"
#include "gui/ButtonWithLabel.h"
#include "gui/AppResources.h"
#include "gui/HorizontalLine.h"
#include "gui/StoryRoleSlotWidget.h"
#include "data/Scenario.h"
#include "util/StringUtils.h"

using namespace fig::chat;
using namespace fig::data;

namespace fig::gui
{
	StoryEditorRolesTab::StoryEditorRolesTab(ControlPtr pParent) : EditorTab(pParent)
	{
		SetMaxWidth(1280);
	}

	bool StoryEditorRolesTab::OnInit(StoryEditorArgs args)
	{
		if (not (bool)args.pScenario)
			return false;

		_pScenario = args.pScenario;

		auto pSizer = SetSizer<VerticalSizer>();

		CreateHeader(this, pSizer, "Roles");

		// Tags
		CreateLabel(this, pSizer, "Number of roles");
		auto pNumRoles = CreateDropList(this, pSizer, [](auto&& value) {
			
		});
		pNumRoles->AddItems(fig::string_list { "2", "3", "4" });

		CreateHorizontalLine(this, pSizer);

		bool bMulti = _pScenario->GetRoleSlots().size() > 2uz;
		int32_t n = 1;
		for (auto& slot : _pScenario->GetRoleSlots())
		{
			if (is_bot(slot.role))
			{
				if (bMulti)
					CreateHeader(this, pSizer, std::format("AI role #{}", n++));
				else
					CreateHeader(this, pSizer, "AI role");
				CreateRole(pSizer, slot.role);
			}
		}
		CreateHeader(this, pSizer, "User role");
		CreateRole(pSizer, Role::User);

		CreateTabGroup();
		return true;
	}

	EditorTabBase::SaveResult StoryEditorRolesTab::OnSave() noexcept
	{
		return {};
	}

	void StoryEditorRolesTab::CreateRole(SizerPtr pSizer, Role role)
	{
		auto pWidget = CreateControl<StoryRoleSlotWidget>(_pScenario.get(), role);
		pSizer->Add(pWidget, 0, SizerFlag::Expand | SizerFlag::Bottom, 8);
	}
}