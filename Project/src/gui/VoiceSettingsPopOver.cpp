#include <pch.h>
#include "gui/VoiceSettingsPopOver.h"
#include "gui/Frame.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/HorizontalLine.h"
#include "gui/Slider.h"
#include "gui/CheckBox.h"
#include "gui/AppResources.h"
#include "user/UserSettings.h"
#include "audio/AudioManager.h"
#include "tts/TTSBackend.h"

using namespace fig::io;

namespace fig::gui
{
	constexpr fig::coord kSpacing = 6;
	constexpr fig::coord kMargin = 12;

	VoiceSettingsPopOver::VoiceSettingsPopOver(FramePtr pFrame) : Overlay(pFrame)
	{
		SetSize(280, 480);
		SetBackgroundColor(Color::Transparent);
		SetForegroundColor(Color::PanelForeground);

		auto pBG = SetBackgroundRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BACKGROUND_10PX, 16);
		pBG->SetColor(fig::color_ref(Color::MenuBackground).WithAlpha(0.8f));

		auto pBorder = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_10PX, 16);
		pBorder->SetColor(Color::Border);

		auto pSizer = SetSizer<VerticalSizer>();

		pSizer->AddSpacer(kMargin);

		auto& userSettings = Global::GetUserSettings();

		// Enabled
		_pEnabled = CreateControl<CheckBox>("Enable text-to-speech");
		_pEnabled->SetValue(userSettings.GetBool(UserSetting::TTS::Enabled));
		_pEnabled->SetDelegate([this](bool bOn) { EnableTTS(bOn); });
		pSizer->Add(_pEnabled, 0, SizerFlag::Left | SizerFlag::Right, kMargin);

		// ----
		pSizer->Add(CreateControl<HorizontalLine>(), 0, SizerFlag::Expand | SizerFlag::Left | SizerFlag::Right, kMargin);

		// Volume
		CreateLabel("Volume");
		_pVolumeSlider = CreateSlider();
		_pVolumeSlider->SetValue(Global::GetUserSettings().GetFloat(UserSetting::TTS::Volume), true);
		_pVolumeSlider->SetDelegate([this](float value) { SetVolume(value); });

		EnableControls(userSettings.GetBool(UserSetting::TTS::Enabled));

		LayoutNow();
		ResizeToFit(false, true);

		SetHeight(GetHeight() + kSpacing + kMargin);

		SetVisible(false);
	}

	uint32_t VoiceSettingsPopOver::Show(fig::rect parentRect)
	{
		if (_pOwner)
			_pOwner->PopAllOverlays();

		auto menuY = parentRect.y + parentRect.h;
		auto menuHeight = GetHeight();
		auto maxY = _pOwner->GetHeight();
		if (menuHeight >= maxY)
			menuY = 0;
		else if (menuY + menuHeight > maxY)
			menuY = parentRect.y - menuHeight;
		menuY = std::max(menuY, 0);

		SetAbsolutePosition(fig::point { parentRect.x, menuY });
		SetVisible(true);

		return _pOwner->PushOverlay(this);
	}

	uint32_t VoiceSettingsPopOver::Show(fig::point position)
	{
		if (_pOwner)
			_pOwner->PopAllOverlays();

		if (position.x == -1 and position.y == -1)
		{
			float mx, my;
			SDL_GetMouseState(&mx, &my);
			position = fig::point { toI(mx), toI(my) };
		}

		SetAbsolutePosition(position);
		SetVisible(true);

		return _pOwner->PushOverlay(this);
	}

	EventResult VoiceSettingsPopOver::OnEvent(fig::event& event)
	{
		if (IsUserEvent(event, UserEvent::Scrolling))
		{
			if (_pOwner)
				_pOwner->PopOverlay(this);
			return EventResult::Continue;
		}
		return EventResult::Pass;
	}

	fig::observer_ptr<StaticText> VoiceSettingsPopOver::CreateLabel(fig::string_view text)
	{
		auto pSizer = GetSizer();
		auto pLabel = CreateControl<StaticText>(fig::string { text }, FontFace::Default, 14.0, false);
		pSizer->Add(pLabel, 0, SizerFlag::Expand | SizerFlag::Left | SizerFlag::Right, kMargin);
		pSizer->AddSpacer(kSpacing);
		return pLabel;
	}

	fig::observer_ptr<Slider> VoiceSettingsPopOver::CreateSlider()
	{
		auto pSizer = GetSizer();
		auto pSlider = CreateControl<Slider>();
		pSizer->Add(pSlider, 0, SizerFlag::Expand | SizerFlag::Left | SizerFlag::Right, kMargin);
		pSizer->AddSpacer(kSpacing + 6);
		return pSlider;
	}

	void VoiceSettingsPopOver::EnableControls(bool bEnabled)
	{
		_pVolumeSlider->SetEnabled(bEnabled);
	}

	void VoiceSettingsPopOver::EnableTTS(bool bEnable)
	{
		EnableControls(bEnable);
		
		Global::GetUserSettings().SetBool(UserSetting::TTS::Enabled, bEnable);
		
		if (not bEnable)
		{
			Global::GetTTSBackend().CancelAll();
			Global::GetAudioManager().StopAllSounds();
		}
	}

	void VoiceSettingsPopOver::SetVolume(float fVolume)
	{
		fVolume = std::clamp(fVolume, 0.0f, 1.0f);
		Global::GetUserSettings().SetFloat(UserSetting::TTS::Volume, fVolume);
		Global::GetAudioManager().SetVolume(fVolume);
	}
}