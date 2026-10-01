#include <pch.h>
#include "gui/UserSettingsEditorVoiceTab.h"
#include "gui/CheckBox.h"
#include "gui/AppResources.h"
#include "gui/Slider.h"
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
		auto pEnabledToggle = CreateToggle(this, pSizer, "Enable voice features", [](auto&& bOn) {
			Global::GetUserSettings().SetBool(UserSetting::TTS::Enabled, bOn);
		});
		pEnabledToggle->SetValue(userSettings.GetBool(UserSetting::TTS::Enabled), false);

		CreateHorizontalLine(this, pSizer);
		CreateHeader(this, pSizer, "Backend configuration");

		// Backend
		CreateLabel(this, pSizer, "Select backend");
		auto ttsBackends = Global::GetTTSBackend().GetBackendSettings();
		auto pTTSBackend = CreateDropList(this, pSizer,
			ttsBackends 
				| std::views::transform([](auto&& b) { return b.name; })
				| std::ranges::to<std::vector>(),
			[ttsBackends](int32_t value) {
				if (value >= 0 and value < toI(ttsBackends.size()))
					Global::GetUserSettings().SetUUID(UserSetting::TTS::Backend, ttsBackends[value].id);
				else
					Global::GetUserSettings().SetUUID(UserSetting::TTS::Backend, {});
			});
		pTTSBackend->SetMaxWidth(340);
		pTTSBackend->SetEnabled(not ttsBackends.empty());
		
		auto currentBackendId = Global::GetUserSettings().GetUUID(UserSetting::TTS::Backend);
		if (auto itBackend = std::ranges::find(ttsBackends, currentBackendId, [](auto&& b) { return b.id; }); itBackend != std::ranges::cend(ttsBackends))
			pTTSBackend->Select(static_cast<int32_t>(std::distance(ttsBackends.begin(), itBackend)), false);
		else
			pTTSBackend->Select(-1, false);

		// Voice model
		CreateLabel(this, pSizer, "Select speech model");
		auto ttsModels = Global::GetTTSBackend().GetVoiceModels();
		auto voiceModels = ttsModels
			| std::views::filter([](auto&& m) { return m.task.task == TTSTask::Speech; })
			| std::ranges::to<std::vector>();
		auto pTTSVoiceModel = CreateDropList(this, pSizer,
			voiceModels
				| std::views::transform([](auto&& b) { return b.name; })
				| std::ranges::to<std::vector>(),
			[voiceModels](int32_t value) {
				if (value >= 0 and value < toI(voiceModels.size()))
					Global::GetUserSettings().SetUUID(UserSetting::TTS::SpeechModel, voiceModels[value].id);
				else
					Global::GetUserSettings().SetUUID(UserSetting::TTS::SpeechModel, {});
			});
		pTTSVoiceModel->SetMaxWidth(340);
		pTTSVoiceModel->SetEnabled(not voiceModels.empty());

		auto currentVoiceModelId = Global::GetUserSettings().GetUUID(UserSetting::TTS::SpeechModel);
		if (auto itModel = std::ranges::find(voiceModels, currentVoiceModelId, [](auto&& b) { return b.id; }); itModel != std::ranges::cend(voiceModels))
			pTTSVoiceModel->Select(static_cast<int32_t>(std::distance(voiceModels.begin(), itModel)), false);
		else
			pTTSVoiceModel->Select(-1, false);

		// Design model
		CreateLabel(this, pSizer, "Select voice design model");
		auto designModels = ttsModels
			| std::views::filter([](auto&& m) { return m.task.task == TTSTask::Design; })
			| std::ranges::to<std::vector>();
		auto pTTSDesignModel = CreateDropList(this, pSizer,
			designModels
				| std::views::transform([](auto&& b) { return b.name; })
				| std::ranges::to<std::vector>(),
			[designModels](int32_t value) {
				if (value >= 0 and value < toI(designModels.size()))
					Global::GetUserSettings().SetUUID(UserSetting::TTS::DesignModel, designModels[value].id);
				else
					Global::GetUserSettings().SetUUID(UserSetting::TTS::DesignModel, {});
			});
		pTTSDesignModel->SetMaxWidth(340);
		pTTSDesignModel->SetEnabled(not designModels.empty());
		
		auto currentDesignModelId = Global::GetUserSettings().GetUUID(UserSetting::TTS::DesignModel);
		if (auto itModel = std::ranges::find(designModels, currentDesignModelId, [](auto&& b) { return b.id; }); itModel != std::ranges::cend(designModels))
			pTTSDesignModel->Select(static_cast<int32_t>(std::distance(designModels.begin(), itModel)), false);
		else
			pTTSDesignModel->Select(-1, false);

		CreateHorizontalLine(this, pSizer);
		CreateHeader(this, pSizer, "Other settings");

		// Volume
		CreateLabel(this, pSizer, "Volume");
		auto pVolume = CreateControl<Slider>(0.0f, 1.0f);
		pVolume->SetMaxWidth(400);
		pVolume->SetValue(Global::GetUserSettings().GetFloat(UserSetting::TTS::Volume), true);
		pVolume->SetDelegate([](auto&& value) {
			Global::GetUserSettings().SetFloat(UserSetting::TTS::Volume, value);
		});
		pSizer->AddSpacer(4);
		pSizer->Add(pVolume, 0, SizerFlag::Expand | SizerFlag::Bottom, 12);

		// Warm-up
		auto pWarmUpToggle = CreateToggle(this, pSizer, "Warm-up TTS backend (faster first response)", [](auto&& bOn) {
			Global::GetUserSettings().SetBool(UserSetting::TTS::Warmup, bOn);
		});
		pWarmUpToggle->SetValue(userSettings.GetBool(UserSetting::TTS::Warmup), false);

		// Split messages
		auto pSplitToggle = CreateToggle(this, pSizer, "Split longer messages", [](auto&& bOn) {
			Global::GetUserSettings().SetBool(UserSetting::TTS::Split, bOn);
		});
		pSplitToggle->SetValue(userSettings.GetBool(UserSetting::TTS::Split), false);

		return true;
	}

	void UserSettingsEditorVoiceTab::OnAfterLayout()
	{
		ResizeToFit(false, true);
	}

	EditorTabBase::SaveResult UserSettingsEditorVoiceTab::OnSave() noexcept
	{
		return {};
	}
}