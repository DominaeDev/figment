#include <pch.h>
#include "gui/EditorScreen.h"
#include "gui/ScrollPanel.h"
#include "gui/TopBar.h"

namespace fig::gui
{
	EditorScreen::EditorScreen(Frame* pParent) : Screen(pParent)
	{
		_pScrollPanel = CreateControl<ScrollPanel>();
		_pScrollPanel->SetScrollBarOffset(0);
		_pScrollPanel->SetBottomPadding(40);
		_pScrollPanel->SetMaxWidth(Constants::GUI::EditorPage::Width);
		_pScrollPanel->SetSizer<VerticalSizer>();

		_pTopBar = CreateControl<TopBar>("", _pScrollPanel);

		auto mainSizer = SetSizer<VerticalSizer>();
		mainSizer->Add(_pTopBar, 0, SizerFlag::Expand);
		mainSizer->Add(_pScrollPanel, -1, SizerFlag::Fill | SizerFlag::AlignLeft | SizerFlag::Left | SizerFlag::Right, 16);
	}

	EditorScreen::~EditorScreen()
	{
		ReleaseEditor();
	}

	void EditorScreen::ReleaseEditor()
	{
		if (_pEditor)
		{
			_pScrollPanel->RemoveChildren();
			_pEditor->Shutdown();
			_pEditor.reset();
		}
	}

	void EditorScreen::SetTitle(fig::string_view text)
	{
		_pTopBar->SetTitle(text);
	}

	void EditorScreen::OnSetEditor()
	{
		// Init top bar
		_pTopBar->Initialize(_pEditor->GetTitle());

		// Init editor
		_pScrollPanel->DestroyChildren();
		_pScrollPanel->AddChild(_pEditor.get());
		auto pSizer = _pScrollPanel->GetSizer();
		pSizer->Add(_pEditor.get(), 0, SizerFlag::Expand);

		_pEditor->PopulateTopBar(_pTopBar);
		_pEditor->SelectPage(0);

		InvalidateLayout();
	}

	bool EditorScreen::OnKeyboardEvent(KeyboardEvent& event)
	{
		return false;
	}

	EventResult EditorScreen::OnEvent(fig::event& event)
	{
		if (IsUserEvent(event, UserEvent::ScreenDeactivated))
		{
			ReleaseEditor();
			return EventResult::Handled;
		}

		return EventResult::Pass;
	}
}