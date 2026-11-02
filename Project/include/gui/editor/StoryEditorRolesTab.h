#pragma once

#include "gui/editor/EditorTab.h"
#include "gui/editor/StoryEditorArgs.h"

namespace fig::gui
{
	class StoryRoleEditWidget;
	class StoryRoleSlotBox;

	class StoryEditorRolesTab : public EditorTab<StoryEditorArgs>
	{
	public:
		StoryEditorRolesTab(ControlPtr pParent);

		SaveResult OnSave() noexcept override;

	protected:
		bool OnInit(StoryEditorArgs args) override;
		void OnTabSelected() override;

		void CreateRole(SizerPtr pBoxSizer, SizerPtr pEditSizer, fig::chat::Role role);
		void RefreshRoles();
		void AddRole();

		void ShowRoleSlotMenu(fig::chat::Role role);
		bool IsCharacterStory() const noexcept;

	private:
		fig::observer_ptr<fig::data::Scenario> _pScenario {};
		fig::observer_ptr<const fig::data::Character> _pCharacter {};

		struct RoleItem
		{
			fig::chat::Role role;
			fig::observer_ptr<StaticText> pHeader;
			fig::observer_ptr<StoryRoleSlotBox> pBox;
			fig::observer_ptr<StoryRoleEditWidget> pEdit;
		};

		std::vector<RoleItem> _roleItems {};
		fig::observer_ptr<ButtonWithIcon> _pAddButton;
	};
}