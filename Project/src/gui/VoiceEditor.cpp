#include <pch.h>
#include "gui/VoiceEditor.h"
#include "gui/ToggleWithLabel.h"
#include "gui/SimpleTextBox.h"
#include "gui/AppResources.h"
#include "gui/ImageViewport.h"
#include "data/Character.h"
#include "audio/AudioManager.h"
#include "tts/TTSBackend.h"

using namespace fig::data;
using namespace fig::tts;
using namespace fig::io;

namespace fig::gui
{
	constexpr fig::string_view examplePhrase = "Greetings! I'm {}, and this is my voice. Do you like it? If not, well, that sounds like a YOU problem.";

	static const fig::handle groupGender = "gender";
	static const fig::handle groupMaturity = "maturity";
	static const fig::handle groupTone = "tone";
	static const fig::handle groupFlow = "flow";
	static const fig::handle groupTemperature = "temperature";
	static const fig::handle groupPresence = "presence";

	static const std::map<fig::handle, std::pair<const fig::string_view, const fig::string_view>> voice_prompts
	{
		// Gender
		{ "gender_female",			std::pair { "female", "" } },
		{ "gender_male",			std::pair { "male", "" } },

		// Maturity
		{ "maturity_spry",			std::pair { "child, small boy", "female, infant, girl" } },
		{ "maturity_daring",		std::pair { "male, teenager", "female, teenager" } },
		{ "maturity_ambitious",		std::pair { "male, age 20", "woman, age 20" } },
		{ "maturity_dependable",	std::pair { "adult man", "adult woman" } },
		{ "maturity_seasoned",		std::pair { "middle-aged man, age 48", "mature, middle-aged woman, age 48" } },
		{ "maturity_venerable",		std::pair { "very old man, crusty, grandfather, raspy, geriatric, age 70", "very old woman, crusty, grandmother, raspy, geriatric, age 70" } },

		// Tone
		{ "tone_textured",			std::pair { "A rough, raspy, hoarse, and gravelly voice", "" } },
		{ "tone_deep",				std::pair { "Deep voice, with low, husky, throaty notes.", "" } },
		{ "tone_mellow",			std::pair { "A mellow, neither masculine nor feminine, mid-range voice", "" } },
		{ "tone_light",				std::pair { "A bright and clear, high pitched voice with distinctly feminine notes", "" } },
		{ "tone_crisp",				std::pair { "Loud, crisp, and optimistic", "" } },
		{ "tone_cute",				std::pair { "An affectionate, kind-hearted voice with light notes and a tender, melodic timbre", "" } },
		{ "tone_soft",				std::pair { "Soft like velvet, a tiny whisper, with a weak presence", "" } },

		// Flow
		{ "flow_relaxed",			std::pair { "Slow, gentle, and relaxed tempo and flow", "" } },
		{ "flow_controlled",		std::pair { "Controlled, precise, and methodical flow", "" } },
		{ "flow_balanced",			std::pair { "Regular tempo and conversational flow", "" } },
		{ "flow_monotone",			std::pair { "Monotone, extremely apathic, emotionless, very flat, extremely stoic, boring, flat intonation, android, cyborg, robot, methodical, articulate", "" } },
		{ "flow_quick",				std::pair { "Talks very fast, in a snappy, impatient pace", "" } },
		
		// Temperature
		{ "temperature_friendly",		std::pair { "Friendly, kind, and easy-going", "" } },
		{ "temperature_playful",		std::pair { "Playful, joyous, and enthusiastic", "" } },
		{ "temperature_sensual",		std::pair { "Sensual, gentle, angelic, and serene voice", "" } },
		{ "temperature_arrogant",		std::pair { "Arrogant, concieted, brash, and overconfident", "" } },
		{ "temperature_protective",		std::pair { "A loving parental figure, with notes of platonic love, guidance, and unyielding protection", "" } },
		{ "temperature_gloomy",			std::pair { "Depressed, gloomy and sad, his voice reflects her fragility and loneliness", "Depressed, gloomy and sad, her voice reflects her fragility and loneliness" } },
		{ "temperature_intimate",		std::pair { "Seductive, alluring, and intimately close, with notes of desire, lust, and growing arousal", "" } },
		{ "temperature_malevolent",		std::pair { "A cruel, spiteful, haughty, villainous timbre, with undertones of pure hatred, disgust, and resentment", "" } },
		
		// Presence
		{ "presence_shy",			std::pair { "His voice reflects his shy timidness and hesitant pacing", "Her voice reflects her shy timidness and hesitant pacing" } },
		{ "presence_nervous",		std::pair { "hesitant, scared, uneasy, frightened, embarrassed, stressed, nervous and scared", "" } },
		{ "presence_grounded",		std::pair { "His voice reflects a grounded, comfortable individual", "Her voice reflects a grounded, comfortable individual" } },
		{ "presence_strong",		std::pair { "His voice reflects a strong, easy-going character", "Her voice reflects a strong, easy-going character" } },
		{ "presence_commanding",	std::pair { "His voice reflects his commanding confidence and domineering character", "Her voice reflects her commanding confidence and domineering character" } },
	};

	VoiceEditor::VoiceEditor(const fig::uuid& characterId) : Editor(nullptr),
		_characterId { characterId }
	{
		if (auto try_character = Global::GetUserContent().Get<Character>(characterId))
			_character = fig::data::Character { *try_character };

		_audioResultQueue.SetDelegate([this](auto&& r) { OnAudioResult(std::move(r)); });
	}

	fig::string VoiceEditor::GetTitle() const noexcept
	{
		return std::format("Configuring {}'s voice", _character.GetName());
	}

	void VoiceEditor::Initialize() noexcept
	{
		auto pSizer = SetSizer<VerticalSizer>();

		auto pHorizontalSizer = new HorizontalSizer();

		auto pDesignerSizer = new VerticalSizer();

		// Gender
		CreateLabel(this, pDesignerSizer, "Pitch");
		auto pGenderSizer = new HorizontalSizer();
		auto pMale = CreateToggle(pGenderSizer, groupGender, "gender_male", "Masculine", true);
		auto pFemale = CreateToggle(pGenderSizer, groupGender, "gender_female", "Feminine", true);
		pDesignerSizer->Add(pGenderSizer);

		// Maturity
		CreateLabel(this, pDesignerSizer, "Maturity");
		auto pMaturitySizer1 = new HorizontalSizer();
		auto pMaturitySizer2 = new HorizontalSizer();
		CreateToggle(pMaturitySizer1, groupMaturity, "maturity_spry", "Spry");
		CreateToggle(pMaturitySizer1, groupMaturity, "maturity_daring", "Daring");
		CreateToggle(pMaturitySizer1, groupMaturity, "maturity_ambitious", "Ambitious");
		CreateToggle(pMaturitySizer1, groupMaturity, "maturity_dependable", "Dependable");
		CreateToggle(pMaturitySizer1, groupMaturity, "maturity_seasoned", "Seasoned");
		CreateToggle(pMaturitySizer2, groupMaturity, "maturity_venerable", "Venerable");
		pDesignerSizer->Add(pMaturitySizer1);
		pDesignerSizer->AddSpacer(6);
		pDesignerSizer->Add(pMaturitySizer2);

		// Tone
		CreateLabel(this, pDesignerSizer, "Tone");
		auto pToneSizer1 = new HorizontalSizer();
		auto pToneSizer2 = new HorizontalSizer();
		CreateToggle(pToneSizer1, groupTone, "tone_deep", "Deep");
		CreateToggle(pToneSizer1, groupTone, "tone_mellow", "Mellow");
		CreateToggle(pToneSizer1, groupTone, "tone_light", "Light");
		CreateToggle(pToneSizer1, groupTone, "tone_soft", "Soft");
		CreateToggle(pToneSizer1, groupTone, "tone_cute", "Sweet");
		CreateToggle(pToneSizer2, groupTone, "tone_textured", "Textured");
		CreateToggle(pToneSizer2, groupTone, "tone_crisp", "Crisp");
		pDesignerSizer->Add(pToneSizer1);
		pDesignerSizer->AddSpacer(6);
		pDesignerSizer->Add(pToneSizer2);
		
		// Presence
		CreateLabel(this, pDesignerSizer, "Presence");
		auto pPresenceSizer1 = new HorizontalSizer();
//		auto pPresenceSizer2 = new HorizontalSizer();
		CreateToggle(pPresenceSizer1, groupPresence, "presence_nervous", "Nervous");
		CreateToggle(pPresenceSizer1, groupPresence, "presence_shy", "Weak");
		CreateToggle(pPresenceSizer1, groupPresence, "presence_grounded", "Grounded");
		CreateToggle(pPresenceSizer1, groupPresence, "presence_strong", "Strong");
		CreateToggle(pPresenceSizer1, groupPresence, "presence_commanding", "Commanding");
		pDesignerSizer->Add(pPresenceSizer1);
//		pDesignerSizer->AddSpacer(6);
//		pDesignerSizer->Add(pPresenceSizer2);

		// Flow
		CreateLabel(this, pDesignerSizer, "Flow");
		auto pFlowSizer = new HorizontalSizer();
		CreateToggle(pFlowSizer, groupFlow, "flow_relaxed", "Relaxed");
		CreateToggle(pFlowSizer, groupFlow, "flow_balanced", "Balanced");
		CreateToggle(pFlowSizer, groupFlow, "flow_controlled", "Controlled");
		CreateToggle(pFlowSizer, groupFlow, "flow_monotone", "Flat");
		CreateToggle(pFlowSizer, groupFlow, "flow_quick", "Quick");
		pDesignerSizer->Add(pFlowSizer);

		// Temperature
		CreateLabel(this, pDesignerSizer, "Mood");
		auto pTemperatureSizer1 = new HorizontalSizer();
		auto pTemperatureSizer2 = new HorizontalSizer();
		CreateToggle(pTemperatureSizer1, groupTemperature, "temperature_friendly", "Friendly");
		CreateToggle(pTemperatureSizer1, groupTemperature, "temperature_playful", "Playful");
		CreateToggle(pTemperatureSizer1, groupTemperature, "temperature_protective", "Protective");
		CreateToggle(pTemperatureSizer1, groupTemperature, "temperature_sensual", "Sensual");
		CreateToggle(pTemperatureSizer1, groupTemperature, "temperature_intimate", "Intimate");
		CreateToggle(pTemperatureSizer2, groupTemperature, "temperature_gloomy", "Gloomy");
		CreateToggle(pTemperatureSizer2, groupTemperature, "temperature_arrogant", "Arrogant");
		CreateToggle(pTemperatureSizer2, groupTemperature, "temperature_malevolent", "Malevolent");
		pDesignerSizer->Add(pTemperatureSizer1);
		pDesignerSizer->AddSpacer(6);
		pDesignerSizer->Add(pTemperatureSizer2);

		// Custom
		CreateLabel(this, pDesignerSizer, "Custom prompt");
		_pCustomPrompt = CreateControl<SimpleTextBox>();
		_pCustomPrompt->SetMaxWidth(532);
		pDesignerSizer->Add(_pCustomPrompt, 0, SizerFlag::Expand);

		pHorizontalSizer->Add(pDesignerSizer, -1, SizerFlag::Expand);
		pSizer->Add(pHorizontalSizer, 0, SizerFlag::Expand);

		_pGenerateButton = CreateControl<ButtonWithLabel>("Generate voice");
		_pGenerateButton->SetHeight(35);
		_pGenerateButton->SetDelegate([this] { Generate(); });
		
		_pPlayButton = CreateControl<ButtonWithIcon>(Resource::ICON_PLAY, true);
		_pPlayButton->SetSize(35, 35);
		_pPlayButton->SetDelegate([this] { PlayStop(); });
		_pPlayButton->Enable(false);

		_pSaveButton = CreateControl<ButtonWithLabel>("Save");
		_pSaveButton->SetHeight(35);
		_pSaveButton->SetDelegate([this] { 
			if (Save())
				PushEvent(UserEvent::NavigateToHome);
		});
		_pSaveButton->Enable(false);

		_pStatusText = CreateControl<StaticText>("");

		auto pButtonSizer = new HorizontalSizer();
		pButtonSizer->Add(_pGenerateButton, 0);
		pButtonSizer->Add(_pPlayButton, 0, SizerFlag::Left, 12);
		pButtonSizer->Add(_pStatusText, -1, SizerFlag::Left | SizerFlag::AlignCenterVertical, 12);

		pDesignerSizer->AddSpacer(24);
		pDesignerSizer->Add(pButtonSizer, 0, SizerFlag::FixedSize, 35);
		pDesignerSizer->AddSpacer(24);
		pDesignerSizer->Add(_pSaveButton, 0);

		ResizeToFit(false, true);

		pMale->Toggle(_character.gender.IsConventional(ConventionalGender::Male));
		pFemale->Toggle(not _character.gender.IsConventional(ConventionalGender::Male));

		_pViewport = CreateControl<ImageViewport>(nullptr, AppResources::GetTexture(Resource::MASK_CARD));
		_pViewport->SetSize(320, 480);
		_pViewport->SetVisible(false);
		pHorizontalSizer->Add(_pViewport, 0, SizerFlag::AlignRight | SizerFlag::Right, 12);

		// Load portrait
		if (auto try_portrait = Global::GetUserContent().GetLargePortraitForCharacter(_characterId))
		{
			if (auto try_image = Global::GetUserContent().GetTexture((*try_portrait).id, GetSDLRenderer()))
			{
				_pViewport->SetTexture((*try_image).get());
				_pViewport->SetVisible(true);
			}
		}

		// Load (existing) voice
		if (auto try_voice = Global::GetUserContent().GetVoiceForCharacter(_characterId))
		{
			_voicePrint.audioData = (*try_voice).voicePrint.audioData;
			_pPlayButton->Enable(true);
		}
	}

	void VoiceEditor::ShutDown() noexcept
	{
		Global::GetAudioManager().StopAllSounds();
		Global::GetTTSBackend().UnloadDesignModels();
	}

	fig::observer_ptr<ToggleWithLabel> VoiceEditor::CreateToggle(SizerPtr pSizer, fig::handle toggleGroup, fig::handle toggleKey, fig::string_view label, bool bRadio)
	{
		auto pToggle = CreateControl<ToggleWithLabel>(label, 14.5, bRadio ? ToggleBehavior::Radio : ToggleBehavior::Default);
		pToggle->SetDelegate([this, toggleGroup, toggleKey](bool bOn) { 
			OnToggle(toggleGroup, toggleKey, bOn);
		});
		pToggle->SetSize(100, 29);
		pSizer->Add(pToggle, 0, SizerFlag::Right, 8);
		_toggleGroups[toggleGroup][toggleKey] = pToggle;
		return pToggle;
	}

	void VoiceEditor::OnToggle(fig::handle group, fig::handle key, bool bOn)
	{
		auto& toggleGroup = _toggleGroups[group];
		if (bOn) // Untoggle others in group
		{
			_selectedKeys.insert(key);
			for (auto& kvp : toggleGroup)
			{
				if (kvp.first != key)
				{
					kvp.second->Toggle(false, true);
					_selectedKeys.erase(kvp.first);
				}
			}
		}
		else
		{
			_selectedKeys.erase(key);
		}
	}

	fig::string VoiceEditor::GetPrompt() const noexcept
	{
		std::vector<fig::string> prompts;

		bool bHasMaturity = false;
		for (auto& key : _toggleGroups.at(groupMaturity) | std::views::keys)
		{
			if (_selectedKeys.contains(key))
			{
				bHasMaturity = true;
				break;
			}
		}

		if (not bHasMaturity)
		{
			if (_selectedKeys.contains("gender_male"))
				prompts.push_back(fig::string { voice_prompts.at("gender_male").first });
			else if (_selectedKeys.contains("gender_female"))
				prompts.push_back(fig::string { voice_prompts.at("gender_female").first });
		}

		if (auto customPrompt = _pCustomPrompt->GetText(); not empty_or_whitespace(customPrompt))
			prompts.push_back(customPrompt);

		static const std::array<fig::handle, 5> groupOrder {
			groupMaturity, groupTone, groupTemperature, groupFlow, groupPresence,
		};
		bool bIsMale = _selectedKeys.contains("gender_male");

		for (auto& groupKey : groupOrder)
		{
			for (auto& key : _toggleGroups.at(groupKey) | std::views::keys)
			{
				if (_selectedKeys.contains(key))
				{
					auto& prompt = (bIsMale or voice_prompts.at(key).second.empty()) ? voice_prompts.at(key).first : voice_prompts.at(key).second;
					if (not prompt.empty())
						prompts.push_back(fig::string { prompt });
				}
			}
		}

		return prompts
			| std::views::join_with(std::string_view { ". " })
			| std::ranges::to<std::string>();
	}

	void VoiceEditor::Generate() noexcept
	{
		fig::string name = _character.GetName();
		if (empty_or_whitespace(name))
			name = "Character";
		fig::string phrase = std::format(examplePhrase, name);
		
		bool isServerRunning = Global::GetTTSBackend().GetStatus() >= TTSStatus::ServerStarted;

		auto prompt = GetPrompt();
		if (auto result = Global::GetTTSBackend().Design(phrase, prompt))
		{
			_voicePrint = {
				.generationPrompt = prompt,
				.referenceText = phrase,
				.keys = _selectedKeys 
					| std::views::transform([](auto&& key) { return (fig::string)key; })
					| std::ranges::to<std::vector>(),
			};

			_audioResultQueue.Add(std::move(result).value());
			_pGenerateButton->Enable(false);
			_pPlayButton->Enable(false);
			_pSaveButton->Enable(false);

			SetStatusMessage(isServerRunning ? fig::strings::TTS::Generating : fig::strings::TTS::ServerInitializing);
			Global::GetAudioManager().StopAllSounds();
		}
		else
		{
			SetStatusMessage(fig::strings::TTS::ServerUnavailable);
		}
	}

	void VoiceEditor::OnUpdate(float fElapsed)
	{
		_audioResultQueue.Update();

		if (bool isPlaying = Global::GetAudioManager().IsPlaying(); isPlaying != _bIsPlaying)
		{
			_bIsPlaying = isPlaying;
			_pPlayButton->SetIcon(isPlaying ? Resource::ICON_STOP : Resource::ICON_PLAY);

			if (isPlaying)
				SetStatusMessage(fig::strings::TTS::PlayingSound);
			else if (_pStatusText->GetText() == fig::strings::TTS::PlayingSound)
				SetStatusMessage("");
		}

		if (not _pGenerateButton->IsEnabled() and _audioResultQueue.IsEmpty())
		{
			_pGenerateButton->Enable(true);
			_pPlayButton->Enable(true);
		}
	}

	void VoiceEditor::PlayStop() noexcept
	{
		if (_bIsPlaying)
		{
			Global::GetAudioManager().StopAllSounds();
		}
		else if (not _voicePrint.audioData.empty())
		{
			// Play
			Global::GetAudioManager().StopAllSounds();
			Global::GetAudioManager().EnqueueSound(_voicePrint.audioData.AsBytes());
		}
	}

	EventResult VoiceEditor::OnEvent(fig::event& event)
	{
		if (IsUserEvent(event, UserEvent::TTSServerLoadingModel))
		{
			SetStatusMessage(fig::strings::TTS::LoadingModel);
			return EventResult::Continue;
		}
		else if (IsUserEvent(event, UserEvent::TTSServerGenerating))
		{
			SetStatusMessage(fig::strings::TTS::Generating);
			return EventResult::Continue;
		}
		else if (IsUserEvent(event, UserEvent::TTSServerError))
		{
			SetStatusMessage(fig::strings::TTS::ErrorOccurred);
			_audioResultQueue.Clear();
			return EventResult::Continue;
		}
		else if (IsUserEvent(event, UserEvent::TTSServerShutdown))
		{
			SetStatusMessage(fig::strings::TTS::ServerUnavailable);
			_audioResultQueue.Clear();
			return EventResult::Continue;
		}
		return EventResult::Pass;
	}

	void VoiceEditor::SetStatusMessage(fig::string_view message)
	{
		_pStatusText->SetText(message);
	}

	bool VoiceEditor::Save() noexcept
	{
		if (_voicePrint.audioData.empty())
			return false;

		VoiceSettings voiceSettings;
		voiceSettings.name = std::format("{}'s voice", _character.GetName());
		voiceSettings.voicePrint = _voicePrint;
		
		auto voiceSettingsId = Global::GetUserContent().CreateVoiceReference(_characterId, voiceSettings);
		if (voiceSettingsId.empty())
			return false;
		return true;
	}

	void VoiceEditor::OnAudioResult(fig::tts::TTSPayload&& payload)
	{
		if (payload.has_value())
		{
			_voicePrint.audioData = std::move(payload).value();
			Global::GetAudioManager().StopAllSounds();
			Global::GetAudioManager().EnqueueSound(_voicePrint.audioData.AsBytes());

			SetStatusMessage("");
			_pPlayButton->Enable(true);
			_pSaveButton->Enable(true);
		}
		else 
		{
			SetStatusMessage(fig::strings::TTS::ErrorOccurred);
		}
	}
}