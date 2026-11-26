#pragma once

#include "gui/Overlay.h"

namespace fig::gui
{
	class VoiceSettingsPopOver : public Overlay
	{
	public:
		VoiceSettingsPopOver(FramePtr pFrame);


		uint32_t Show(fig::point position = { -1, -1 });
		uint32_t Show(fig::rect parentRect);

	protected:
		EventResult OnEvent(fig::event& event) override;
		
		fig::observer_ptr<StaticText> CreateLabel(fig::string_view text);
		fig::observer_ptr<Slider> CreateSlider();
		void EnableControls(bool bEnabled);
		void EnableTTS(bool bEnable);

		void SetVolume(float fVolume);

	private:
		fig::observer_ptr<CheckBox> _pEnabled;
		fig::observer_ptr<Slider> _pVolumeSlider;
	};
}