#include <pch.h>
#include "gui/VoiceEditor.h"
#include "gui/ToggleWithLabel.h"
#include "gui/SimpleTextBox.h"
#include "gui/AppResources.h"
#include "data/Character.h"
#include "audio/AudioManager.h"

using namespace fig::data;
using namespace fig::tts;

namespace fig::gui
{
	constexpr fig::string_view examplePhrase = "Greetings! My name is {} and this is my voice. Do you like it? If not, I'm sure I can change your mind.";

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
//		SetBackgroundColor(Color::Debug);

		if (auto try_character = Global::GetUserContent().Get<Character>(characterId))
			_character = fig::data::Character { *try_character };
	}

	fig::string VoiceEditor::GetTitle() const noexcept
	{
		return std::format("Configure {}'s voice", _character.GetName());
	}

	void VoiceEditor::Initialize() noexcept
	{
		auto pSizer = SetSizer<VerticalSizer>();

		// Gender
		CreateLabel(this, pSizer, "Pitch");
		auto pGenderSizer = new HorizontalSizer();
		auto pMale = CreateToggle(pGenderSizer, groupGender, "gender_male", "Masculine", true);
		auto pFemale = CreateToggle(pGenderSizer, groupGender, "gender_female", "Feminine", true);
		pSizer->Add(pGenderSizer);

		// Maturity
		CreateLabel(this, pSizer, "Maturity");
		auto pMaturitySizer1 = new HorizontalSizer();
		auto pMaturitySizer2 = new HorizontalSizer();
		CreateToggle(pMaturitySizer1, groupMaturity, "maturity_spry", "Spry");
		CreateToggle(pMaturitySizer1, groupMaturity, "maturity_daring", "Daring");
		CreateToggle(pMaturitySizer1, groupMaturity, "maturity_ambitious", "Ambitious");
		CreateToggle(pMaturitySizer1, groupMaturity, "maturity_dependable", "Dependable");
		CreateToggle(pMaturitySizer1, groupMaturity, "maturity_seasoned", "Seasoned");
		CreateToggle(pMaturitySizer2, groupMaturity, "maturity_venerable", "Venerable");
		pSizer->Add(pMaturitySizer1);
		pSizer->AddSpacer(6);
		pSizer->Add(pMaturitySizer2);

		// Tone
		CreateLabel(this, pSizer, "Tone");
		auto pToneSizer1 = new HorizontalSizer();
		auto pToneSizer2 = new HorizontalSizer();
		CreateToggle(pToneSizer1, groupTone, "tone_deep", "Deep");
		CreateToggle(pToneSizer1, groupTone, "tone_mellow", "Mellow");
		CreateToggle(pToneSizer1, groupTone, "tone_light", "Light");
		CreateToggle(pToneSizer1, groupTone, "tone_soft", "Soft");
		CreateToggle(pToneSizer1, groupTone, "tone_cute", "Sweet");
		CreateToggle(pToneSizer2, groupTone, "tone_textured", "Textured");
		CreateToggle(pToneSizer2, groupTone, "tone_crisp", "Crisp");
		pSizer->Add(pToneSizer1);
		pSizer->AddSpacer(6);
		pSizer->Add(pToneSizer2);
		
		// Presence
		CreateLabel(this, pSizer, "Presence");
		auto pPresenceSizer1 = new HorizontalSizer();
//		auto pPresenceSizer2 = new HorizontalSizer();
		CreateToggle(pPresenceSizer1, groupPresence, "presence_nervous", "Nervous");
		CreateToggle(pPresenceSizer1, groupPresence, "presence_shy", "Weak");
		CreateToggle(pPresenceSizer1, groupPresence, "presence_grounded", "Grounded");
		CreateToggle(pPresenceSizer1, groupPresence, "presence_strong", "Strong");
		CreateToggle(pPresenceSizer1, groupPresence, "presence_commanding", "Commanding");
		pSizer->Add(pPresenceSizer1);
//		pSizer->AddSpacer(6);
//		pSizer->Add(pPresenceSizer2);

		// Flow
		CreateLabel(this, pSizer, "Flow");
		auto pFlowSizer = new HorizontalSizer();
		CreateToggle(pFlowSizer, groupFlow, "flow_relaxed", "Relaxed");
		CreateToggle(pFlowSizer, groupFlow, "flow_balanced", "Balanced");
		CreateToggle(pFlowSizer, groupFlow, "flow_controlled", "Controlled");
		CreateToggle(pFlowSizer, groupFlow, "flow_monotone", "Flat");
		CreateToggle(pFlowSizer, groupFlow, "flow_quick", "Quick");
		pSizer->Add(pFlowSizer);

		// Temperature
		CreateLabel(this, pSizer, "Mood");
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
		pSizer->Add(pTemperatureSizer1);
		pSizer->AddSpacer(6);
		pSizer->Add(pTemperatureSizer2);

		// Custom
		CreateLabel(this, pSizer, "Custom prompt");
		_pCustomPrompt = CreateControl<SimpleTextBox>();
		_pCustomPrompt->SetMaxWidth(532);
		pSizer->Add(_pCustomPrompt, 0, SizerFlag::Expand);

		pSizer->AddSpacer(24);

		_pGenerateButton = CreateControl<ButtonWithLabel>("Generate voice");
		_pGenerateButton->SetHeight(35);
		_pGenerateButton->SetDelegate([this] { Generate(); });
		
		_pPlayButton = CreateControl<ButtonWithIcon>(Resource::ICON_PLAY, true);
		_pPlayButton->SetSize(35, 35);
		_pPlayButton->SetDelegate([this] { PlayStop(); });
		_pPlayButton->Enable(false);

		_pStatusText = CreateControl<StaticText>("");

		auto pButtonSizer = new HorizontalSizer();
		pButtonSizer->Add(_pGenerateButton, 0);
		pButtonSizer->Add(_pPlayButton, 0, SizerFlag::Left, 12);
		pButtonSizer->Add(_pStatusText, -1, SizerFlag::Left | SizerFlag::AlignCenterVertical, 12);

		pSizer->Add(pButtonSizer, 0, SizerFlag::FixedSize, 35);

		ResizeToFit(false, true);

		pMale->Toggle(_character.gender.IsConventional(ConventionalGender::Male));
		pFemale->Toggle(not _character.gender.IsConventional(ConventionalGender::Male));

		
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
				.prompt = prompt,
				.keys = _selectedKeys,
			};

			_pendingResults.emplace_back(std::move(result).value());
			_pGenerateButton->Enable(false);
			_pPlayButton->Enable(false);

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
		// Resolve pending results
		if (not _pendingResults.empty())
		{
			for (auto& result : _pendingResults)
			{
				auto& future = result.future;
				if (not future.valid())
					continue;

				if (future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
				{
					if (auto payload = future.get())
					{
						_voicePrint.audioData = payload.value();
						Global::GetAudioManager().StopAllSounds();
						Global::GetAudioManager().EnqueueSound(_voicePrint.audioData);

						SetStatusMessage("");
						_pPlayButton->Enable(true);
					}
					else
					{
						SetStatusMessage(fig::strings::TTS::ErrorOccurred);
					}
				}
			}

			std::erase_if(_pendingResults, [](auto&& r) { return not r.future.valid(); });
		}

		if (bool isPlaying = Global::GetAudioManager().IsPlaying(); isPlaying != _bIsPlaying)
		{
			_bIsPlaying = isPlaying;
			_pPlayButton->SetIcon(isPlaying ? Resource::ICON_STOP : Resource::ICON_PLAY);

			if (isPlaying)
				SetStatusMessage(fig::strings::TTS::PlayingSound);
			else if (_pStatusText->GetText() == fig::strings::TTS::PlayingSound)
				SetStatusMessage("");
		}

		if (not _pGenerateButton->IsEnabled() and _pendingResults.empty())
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
			Global::GetAudioManager().EnqueueSound(_voicePrint.audioData);
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
			_pendingResults.clear();
			return EventResult::Continue;
		}
		else if (IsUserEvent(event, UserEvent::TTSServerShutdown))
		{
			SetStatusMessage(fig::strings::TTS::ServerUnavailable);
			_pendingResults.clear();
			return EventResult::Continue;
		}
		return EventResult::Pass;
	}

	void VoiceEditor::SetStatusMessage(fig::string_view message)
	{
		_pStatusText->SetText(message);
	}
}