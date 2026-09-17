#pragma once

#include "gui/EditorTab.h"
#include "tts/VoicePrint.h"
#include "tts/AudioResultQueue.h"
#include "gui/CharacterEditorArgs.h"

namespace fig::gui
{
	class CharacterEditorVoiceTab : public EditorTab<CharacterEditorArgs>
	{
	public:
		CharacterEditorVoiceTab(ControlPtr pParent);

		bool Initialize(CharacterEditorArgs args) override;

		SaveResult OnSave() noexcept override;
		void OnShutdown() noexcept override;

	protected:
		void OnUpdate(float fElapsed);
		EventResult OnEvent(fig::event& event) override;
		void OnToggle(fig::handle group, fig::handle key, bool bOn);
		void OnAfterLayout();

		fig::observer_ptr<Sizer> CreateGroup(ControlPtr pParent, SizerPtr pSizer, fig::string_view text);

	private:
		fig::observer_ptr<class ToggleWithLabel> CreateToggle(SizerPtr pSizer, fig::handle toggleGroup, fig::handle toggleKey, fig::string_view label, bool bRadio = false);
		fig::string GetPrompt() const noexcept;
		
		void Generate() noexcept;
		void PlayStop() noexcept;
		void SetStatusMessage(fig::string_view message);
		void OnAudioResult(fig::tts::TTSPayload&& payload);

	private:
		fig::uuid _characterId {};
		fig::observer_ptr<fig::data::Character> _pCharacter {};

		fig::observer_ptr<class ButtonWithLabel> _pGenerateButton;
		fig::observer_ptr<class ButtonWithIcon> _pPlayButton;
		fig::observer_ptr<class ButtonWithLabelAndIcon> _pSaveButton;
		fig::observer_ptr<class ButtonWithLabelAndIcon> _pDiscardButton;

		fig::observer_ptr<class TextInput> _pCustomPrompt;
		fig::observer_ptr<class StaticText> _pStatusText;

		using ToggleGroup = std::map<fig::handle, fig::observer_ptr<class ToggleWithLabel>>;
		std::map<fig::handle, ToggleGroup> _toggleGroups;

		std::unordered_set<fig::handle> _selectedKeys;

		fig::tts::AudioResultQueue _audioResultQueue {};
		fig::tts::VoicePrint _voicePrint;
		bool _bIsPlaying = false;

	};
}