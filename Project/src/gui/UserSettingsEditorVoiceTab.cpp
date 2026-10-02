#include <pch.h>
#include "gui/UserSettingsEditorVoiceTab.h"
#include "gui/CheckBox.h"
#include "gui/AppResources.h"
#include "gui/Slider.h"
#include "gui/ToggleWithLabel.h"
#include "io/PackageManager.h"
#include "util/StringUtils.h"
#include "tts/TTSBackend.h"
#include "tts/TTSBackendSettings.h"

using namespace fig::io;
using namespace fig::data;
using namespace fig::tts;

namespace fig::gui
{
	UserSettingsEditorVoiceTab::UserSettingsEditorVoiceTab(ControlPtr pParent) : EditorTab(pParent)
	{
		SetMaxWidth(1280);
	}

	bool UserSettingsEditorVoiceTab::Initialize(UserSettingsEditorArgs args)
	{
		auto& userSettings = Global::GetUserSettings();
		auto pSizer = SetSizer<VerticalSizer>();

		// Enabled?
		_pEnabledToggle = CreateToggle(this, pSizer, "Enable text-to-speech features", [this](auto&& bOn) {
			Global::GetUserSettings().SetBool(UserSetting::TTS::Enabled, bOn);
			Refresh(); // Toggle controls
		});

		CreateHorizontalLine(this, pSizer);
		CreateHeader(this, pSizer, "Backend configuration");

		// Backend
		CreateLabel(this, pSizer, "Select backend");
		
		_pTTSBackend = CreateDropList(this, pSizer,
			[](int32_t value) {
				auto ttsBackends = Global::GetTTSBackend().GetBackendSettings();
				if (value >= 0 and value < toI(ttsBackends.size()))
					Global::GetUserSettings().SetUUID(UserSetting::TTS::Backend, ttsBackends[value].id);
				else
					Global::GetUserSettings().SetUUID(UserSetting::TTS::Backend, {});
			});
		_pTTSBackend->SetMaxWidth(340);

		// Voice model
		CreateLabel(this, pSizer, "Select speech model");
		_pTTSVoiceModel = CreateDropList(this, pSizer,
			[](int32_t value) {
				auto voiceModels = Global::GetTTSBackend().GetVoiceModels()
					| std::views::filter([](auto&& m) { return m.task.task == TTSTask::Speech; })
					| std::ranges::to<std::vector>();

				if (value >= 0 and value < toI(voiceModels.size()))
					Global::GetUserSettings().SetUUID(UserSetting::TTS::SpeechModel, voiceModels[value].id);
				else
					Global::GetUserSettings().SetUUID(UserSetting::TTS::SpeechModel, {});
			});
		_pTTSVoiceModel->SetMaxWidth(340);

		// Design model
		CreateLabel(this, pSizer, "Select voice design model*");
		
		_pTTSDesignModel = CreateDropList(this, pSizer,
			[](int32_t value) {
				auto designModels = Global::GetTTSBackend().GetVoiceModels()
					| std::views::filter([](auto&& m) { return m.task.task == TTSTask::Design; })
					| std::ranges::to<std::vector>();
				if (value >= 0 and value < toI(designModels.size()))
					Global::GetUserSettings().SetUUID(UserSetting::TTS::DesignModel, designModels[value].id);
				else
					Global::GetUserSettings().SetUUID(UserSetting::TTS::DesignModel, {});
			});
		_pTTSDesignModel->SetMaxWidth(340);
		
		CreateHint(this, pSizer, "* A voice design model is only necessary when using the character voice editor, not for chatting.");

		CreateHorizontalLine(this, pSizer);
		CreateHeader(this, pSizer, "Voice settings");

		// Volume
		CreateLabel(this, pSizer, "Volume");
		_pVolumeSlider = CreateControl<Slider>(0.0f, 1.0f);
		_pVolumeSlider->SetMaxWidth(400);
		_pVolumeSlider->SetValue(Global::GetUserSettings().GetFloat(UserSetting::TTS::Volume), true);
		_pVolumeSlider->SetDelegate([](auto&& value) {
			Global::GetUserSettings().SetFloat(UserSetting::TTS::Volume, value);
		});
		pSizer->AddSpacer(4);
		pSizer->Add(_pVolumeSlider, 0, SizerFlag::Expand | SizerFlag::Bottom | SizerFlag::Left, 6);
		pSizer->AddSpacer(6);

		// Warm-up
		_pWarmUpToggle = CreateToggle(this, pSizer, "Warm-up TTS backend (faster first response)", [](auto&& bOn) {
			Global::GetUserSettings().SetBool(UserSetting::TTS::Warmup, bOn);
		});
		_pWarmUpToggle->SetValue(userSettings.GetBool(UserSetting::TTS::Warmup), false);

		// Split messages
		_pSplitToggle = CreateToggle(this, pSizer, "Split longer messages", [](auto&& bOn) {
			Global::GetUserSettings().SetBool(UserSetting::TTS::Split, bOn);
		});
		_pSplitToggle->SetValue(userSettings.GetBool(UserSetting::TTS::Split), false);

		Refresh();
		return true;
	}

	void UserSettingsEditorVoiceTab::OnTabSelected()
	{
		Refresh();
	}

	void UserSettingsEditorVoiceTab::Refresh()
	{
		auto pSizer = GetSizer();

		auto& userSettings = Global::GetUserSettings();

		// Enabled
		bool bEnabled = userSettings.GetBool(UserSetting::TTS::Enabled);
		_pEnabledToggle->SetValue(bEnabled, true);
		
		// Backend
		auto ttsBackends = Global::GetTTSBackend().GetBackendSettings();
		auto currentBackendId = Global::GetUserSettings().GetUUID(UserSetting::TTS::Backend);
		bEnabled &= not ttsBackends.empty();
		_pEnabledToggle->SetEnabled(not ttsBackends.empty());

		_pTTSBackend->Clear();
		_pTTSBackend->AddItems(ttsBackends
			| std::views::transform([](auto&& b) { return b.name; })
			| std::ranges::to<std::vector>());
		
		if (auto itBackend = std::ranges::find(ttsBackends, currentBackendId, [](auto&& b) { return b.id; }); itBackend != std::ranges::cend(ttsBackends))
			_pTTSBackend->Select(static_cast<int32_t>(std::distance(ttsBackends.begin(), itBackend)), true);
		else
			_pTTSBackend->Select(-1, true);
		
		_pTTSBackend->SetEnabled(bEnabled);

		// Voice model
		auto ttsModels = Global::GetTTSBackend().GetVoiceModels();
		auto currentVoiceModelId = Global::GetUserSettings().GetUUID(UserSetting::TTS::SpeechModel);
		auto voiceModels = ttsModels
			| std::views::filter([](auto&& m) { return m.task.task == TTSTask::Speech; })
			| std::ranges::to<std::vector>();
		_pTTSVoiceModel->Clear();
		_pTTSVoiceModel->AddItems(voiceModels
			| std::views::transform([](auto&& b) { return b.name; })
			| std::ranges::to<std::vector>());

		if (auto itModel = std::ranges::find(voiceModels, currentVoiceModelId, [](auto&& b) { return b.id; }); itModel != std::ranges::cend(voiceModels))
			_pTTSVoiceModel->Select(static_cast<int32_t>(std::distance(voiceModels.begin(), itModel)), true);
		else
			_pTTSVoiceModel->Select(-1, true);
		
		_pTTSVoiceModel->SetEnabled(not voiceModels.empty() and bEnabled);

		// Design model
		auto designModels = ttsModels
			| std::views::filter([](auto&& m) { return m.task.task == TTSTask::Design; })
			| std::ranges::to<std::vector>();
		_pTTSDesignModel->Clear();
		_pTTSDesignModel->AddItems(designModels
			| std::views::transform([](auto&& b) { return b.name; })
			| std::ranges::to<std::vector>());

		auto currentDesignModelId = Global::GetUserSettings().GetUUID(UserSetting::TTS::DesignModel);
		if (auto itModel = std::ranges::find(designModels, currentDesignModelId, [](auto&& b) { return b.id; }); itModel != std::ranges::cend(designModels))
			_pTTSDesignModel->Select(static_cast<int32_t>(std::distance(designModels.begin(), itModel)), true);
		else
			_pTTSDesignModel->Select(-1, true);

		_pTTSDesignModel->SetEnabled(not designModels.empty() and bEnabled);

		_pVolumeSlider->SetValue(Global::GetUserSettings().GetFloat(UserSetting::TTS::Volume), true);
		_pVolumeSlider->SetEnabled(bEnabled);
		_pWarmUpToggle->SetValue(userSettings.GetBool(UserSetting::TTS::Warmup), true);
		_pWarmUpToggle->SetEnabled(bEnabled);
		_pSplitToggle->SetValue(userSettings.GetBool(UserSetting::TTS::Split), true);
		_pSplitToggle->SetEnabled(bEnabled);
	}
}