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
	
	protected:
		void Refresh();
		void OnTabSelected() override;

	private:
		fig::observer_ptr<DropList> _pTTSBackend;
		fig::observer_ptr<DropList> _pTTSVoiceModel;
		fig::observer_ptr<DropList> _pTTSDesignModel;
		fig::observer_ptr<Slider> _pVolumeSlider;
		fig::observer_ptr<CheckBox> _pEnabledToggle;
		fig::observer_ptr<CheckBox> _pWarmUpToggle;
		fig::observer_ptr<CheckBox> _pSplitToggle;

	};
}