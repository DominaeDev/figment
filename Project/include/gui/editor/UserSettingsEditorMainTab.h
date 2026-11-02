#pragma once

#include "gui/editor/EditorTab.h"
#include "gui/editor/UserSettingsEditorArgs.h"

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