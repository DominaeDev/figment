#pragma once

#include "gui/EditorTab.h"
#include "gui/UserSettingsEditorArgs.h"

namespace fig::gui
{
	class UserSettingsEditorMainTab : public EditorTab<UserSettingsEditorArgs>
	{
	public:
		UserSettingsEditorMainTab(ControlPtr pParent);

	protected:
		bool OnInit(UserSettingsEditorArgs args) override;

	protected:
		void ChangeColorTheme(ColorTheme index);
	};
}