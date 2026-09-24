#pragma once

#include "gui/EditorTab.h"
#include "gui/UserSettingsEditorArgs.h"

namespace fig::gui
{
	class UserSettingsEditorVoiceTab : public EditorTab<UserSettingsEditorArgs>
	{
	public:
		UserSettingsEditorVoiceTab(ControlPtr pParent);

		bool Initialize(UserSettingsEditorArgs args) override;
		SaveResult OnSave() noexcept override;

	protected:
		void OnAfterLayout();
		void InstallTTSServer();
	};
}