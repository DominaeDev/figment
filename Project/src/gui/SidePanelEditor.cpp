#include <pch.h>
#include "gui/SidePanelEditor.h"
#include "gui/AppResources.h"
#include "gui/SidePanelButton.h"
#include "gui/MainFrame.h"
#include "gui/EditorScreen.h"

namespace fig::gui
{
	SidePanelEditor::SidePanelEditor(ControlPtr pParent) : SidePanelContent(pParent)
	{
	}

	void SidePanelEditor::ShowExpanded()
	{
		DestroyChildren();

		// Back button
		auto pBackButton = CreateControl<ButtonWithIcon>(Resource::ICON_EXPAND_ARROW_LEFT);
		pBackButton->SetTheme(Theme::SidePanelButtonStyle);
		pBackButton->SetX(3);
		pBackButton->SetY((Constants::GUI::SidePanel::HeaderHeight - pBackButton->GetHeight()) / 2);
		pBackButton->SetDelegate([this]() { PushEvent(UserEvent::NavigateToHome); });

		// Character info
		auto pInfoButton = CreateControl<SidePanelButton>(Resource::ICON_CHARACTER_EDIT_INFO, "General");
		pInfoButton->SetDelegate([]{ PushEvent(UserEvent::SelectEditorPage, 0); });

		// Images
		auto pImagesButton = CreateControl<SidePanelButton>(Resource::ICON_CHARACTER_EDIT_IMAGES, "Images");

		// Voice
		auto pVoiceButton = CreateControl<SidePanelButton>(Resource::ICON_CHARACTER_EDIT_VOICE, "Voice");
		pVoiceButton->SetDelegate([]{ PushEvent(UserEvent::SelectEditorPage, 1); });

		// Story
		auto pStoryButton = CreateControl<SidePanelButton>(Resource::ICON_CHARACTER_EDIT_STORY, "Story");

		// Concepts
		auto pConceptsButton = CreateControl<SidePanelButton>(Resource::ICON_CHARACTER_EDIT_CONCEPTS, "Concepts");

		// Memories
		auto pMemoriesButton = CreateControl<SidePanelButton>(Resource::ICON_CHARACTER_EDIT_MEMORIES, "Memories");

		// About
		auto pAboutButton = CreateControl<SidePanelButton>(Resource::ICON_CHARACTER_EDIT_ABOUT, "About");
		pAboutButton->SetDelegate([]{ PushEvent(UserEvent::SelectEditorPage, 2); });

		auto pTopSizer = SetSizer<VerticalSizer>();
		pTopSizer->AddSpacer(56);
		pTopSizer->Add(pInfoButton, 0, SizerFlag::Expand | SizerFlag::Right | SizerFlag::Left, 12);
		pTopSizer->AddSpacer(4);
		pTopSizer->Add(pImagesButton, 0, SizerFlag::Expand | SizerFlag::Right | SizerFlag::Left, 12);
		pTopSizer->AddSpacer(4);
		pTopSizer->Add(pVoiceButton, 0, SizerFlag::Expand | SizerFlag::Right | SizerFlag::Left, 12);
		pTopSizer->AddSpacer(4);
		pTopSizer->Add(pStoryButton, 0, SizerFlag::Expand | SizerFlag::Right | SizerFlag::Left, 12);
		pTopSizer->AddSpacer(4);
		pTopSizer->Add(pConceptsButton, 0, SizerFlag::Expand | SizerFlag::Right | SizerFlag::Left, 12);
		pTopSizer->AddSpacer(4);
		pTopSizer->Add(pMemoriesButton, 0, SizerFlag::Expand | SizerFlag::Right | SizerFlag::Left, 12);
		pTopSizer->AddSpacer(4);
		pTopSizer->Add(pAboutButton, 0, SizerFlag::Expand | SizerFlag::Right | SizerFlag::Left, 12);
	}

	void SidePanelEditor::ShowCollapsed()
	{
		DestroyChildren();

		// Back button
		auto pBackButton = CreateControl<ButtonWithIcon>(Resource::ICON_EXPAND_ARROW_LEFT);
		pBackButton->SetTheme(Theme::SidePanelButtonStyle);
		pBackButton->SetX(3);
		pBackButton->SetY((Constants::GUI::SidePanel::HeaderHeight - pBackButton->GetHeight()) / 2);
		pBackButton->SetDelegate([this]() { PushEvent(UserEvent::NavigateToHome); });

		// Character info
		auto pInfoButton = CreateControl<ButtonWithIcon>(Resource::ICON_CHARACTER_EDIT_INFO_SMALL, false);
		pInfoButton->SetTheme(Theme::SidePanelButtonStyle);
		pInfoButton->SetDelegate([]() { PushEvent(UserEvent::SelectEditorPage, 0); });

		// Images
		auto pImagesButton = CreateControl<ButtonWithIcon>(Resource::ICON_CHARACTER_EDIT_IMAGES_SMALL, false);
		pImagesButton->SetTheme(Theme::SidePanelButtonStyle);

		// Voice
		auto pVoiceButton = CreateControl<ButtonWithIcon>(Resource::ICON_CHARACTER_EDIT_VOICE_SMALL, false);
		pVoiceButton->SetTheme(Theme::SidePanelButtonStyle);
		pVoiceButton->SetDelegate([]() { PushEvent(UserEvent::SelectEditorPage, 1); });

		// Story
		auto pStoryButton = CreateControl<ButtonWithIcon>(Resource::ICON_CHARACTER_EDIT_STORY_SMALL, false);
		pStoryButton->SetTheme(Theme::SidePanelButtonStyle);

		// Concepts
		auto pConceptsButton = CreateControl<ButtonWithIcon>(Resource::ICON_CHARACTER_EDIT_CONCEPTS_SMALL, false);
		pConceptsButton->SetTheme(Theme::SidePanelButtonStyle);

		// Memories
		auto pMemoriesButton = CreateControl<ButtonWithIcon>(Resource::ICON_CHARACTER_EDIT_MEMORIES_SMALL, false);
		pMemoriesButton->SetTheme(Theme::SidePanelButtonStyle);

		// About
		auto pAboutButton = CreateControl<ButtonWithIcon>(Resource::ICON_CHARACTER_EDIT_ABOUT_SMALL, false);
		pAboutButton->SetTheme(Theme::SidePanelButtonStyle);
		pAboutButton->SetDelegate([]() { PushEvent(UserEvent::SelectEditorPage, 2); });

		auto pTopSizer = SetSizer<VerticalSizer>();
		pTopSizer->AddSpacer(62);
		pTopSizer->Add(pInfoButton, 0, SizerFlag::AlignCenterHorizontal);
		pTopSizer->AddSpacer(8);
		pTopSizer->Add(pImagesButton, 0, SizerFlag::AlignCenterHorizontal);
		pTopSizer->AddSpacer(8);
		pTopSizer->Add(pVoiceButton, 0, SizerFlag::AlignCenterHorizontal);
		pTopSizer->AddSpacer(8);
		pTopSizer->Add(pStoryButton, 0, SizerFlag::AlignCenterHorizontal);
		pTopSizer->AddSpacer(8);
		pTopSizer->Add(pConceptsButton, 0, SizerFlag::AlignCenterHorizontal);
		pTopSizer->AddSpacer(8);
		pTopSizer->Add(pMemoriesButton, 0, SizerFlag::AlignCenterHorizontal);
		pTopSizer->AddSpacer(8);
		pTopSizer->Add(pAboutButton, 0, SizerFlag::AlignCenterHorizontal);

	}

}