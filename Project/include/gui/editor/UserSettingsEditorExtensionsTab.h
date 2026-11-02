#pragma once

#include "gui/editor/EditorTab.h"
#include "gui/editor/UserSettingsEditorArgs.h"

namespace fig::gui
{
	class UserSettingsEditorExtensionsTab : public EditorTab<UserSettingsEditorArgs>
	{
	public:
		UserSettingsEditorExtensionsTab(ControlPtr pParent);

	protected:
		bool OnInit(UserSettingsEditorArgs args) override;

	private:
		std::vector<fig::observer_ptr<class PackageWidget>> _widgets;
	};
}