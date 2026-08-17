#include <pch.h>
#include "gui/EditorScreen.h"
#include "gui/ScrollPanel.h"

namespace fig::gui
{
	EditorScreen::EditorScreen(Frame* pParent) : Screen(pParent)
	{
		auto pTopBar = CreateControl<Panel>();
		pTopBar->SetHeight(Constants::GUI::SidePanel::HeaderHeight);

		_pTitle = pTopBar->CreateControl<StaticText>("", FontFace::Italic, 24, false);
		_pTitle->SetX(52);
		_pTitle->SetHeight(Constants::GUI::SidePanel::HeaderHeight);
		_pTitle->SetAlignment(TextAlignment::LeftCenter);

		auto pTopSizer = pTopBar->SetSizer<HorizontalSizer>();
		pTopSizer->Add(_pTitle, 0, SizerFlag::AlignCenterVertical | SizerFlag::Left, 18);

		_pScrollPanel = CreateControl<ScrollPanel>();
		_pScrollPanel->SetScrollBarOffset(0);
		_pScrollPanel->SetMaxWidth(Constants::GUI::Editor::Width);
		_pScrollPanel->SetSizer<VerticalSizer>();

		auto mainSizer = SetSizer<VerticalSizer>();
		mainSizer->Add(pTopBar, 0, SizerFlag::Expand);
		mainSizer->Add(_pScrollPanel, -1, SizerFlag::Fill | SizerFlag::AlignLeft | SizerFlag::Left | SizerFlag::Right, 16);
	}

	void EditorScreen::ReleaseEditor()
	{
		_pEditor->ShutDown();
		_pEditor.reset();
	}

	void EditorScreen::SetTitle(fig::string_view text)
	{
		_pTitle->SetText(text);
	}

	void EditorScreen::OnSetEditor()
	{
		SetTitle(_pEditor->GetTitle());

		// Create fields
		_pScrollPanel->DestroyChildren();
		_pScrollPanel->AddChild(_pEditor.get());
		_pEditor->Initialize();

		auto pSizer = _pScrollPanel->GetSizer();
		pSizer->Add(_pEditor.get(), 0, SizerFlag::Expand);
		InvalidateLayout();
	}

	bool EditorScreen::OnKeyboardEvent(KeyboardEvent& event)
	{
		return false;
	}

	EventResult EditorScreen::OnEvent(fig::event& event)
	{
		if (IsUserEvent(event, UserEvent::Deactivated))
		{
			ReleaseEditor();
			return EventResult::Handled;
		}

		return EventResult::Pass;
	}
}