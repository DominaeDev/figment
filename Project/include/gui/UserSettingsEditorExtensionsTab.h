#pragma once

#include "gui/EditorTab.h"
#include "gui/UserSettingsEditorArgs.h"

namespace fig::gui
{
	class UserSettingsEditorExtensionsTab : public EditorTab<UserSettingsEditorArgs>
	{
	public:
		UserSettingsEditorExtensionsTab(ControlPtr pParent);

		bool Initialize(UserSettingsEditorArgs args) override;

	private:
		std::vector<fig::observer_ptr<class PackageWidget>> _widgets;
	};
}