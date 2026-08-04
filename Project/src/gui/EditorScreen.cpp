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
		_fields = _pEditor->GetFields();
		_pScrollPanel->DestroyChildren();

		auto pSizer = _pScrollPanel->GetSizer();
		for (auto& field_ptr : _fields)
		{
			if (auto pHeader = std::get_if<std::shared_ptr<EditorHeader>>(&field_ptr))
			{
				auto pLabel = _pScrollPanel->CreateControl<StaticText>(fig::string { (*pHeader)->label}, FontFace::Default, 18.5, false);
				pSizer->Add(pLabel, 0, SizerFlag::Expand | SizerFlag::Top, 8);
			}
			else if (auto pField = std::get_if<std::shared_ptr<IEditorField>>(&field_ptr))
			{
				auto pLabel = _pScrollPanel->CreateControl<StaticText>(fig::string { (*pField)->GetLabel() }, FontFace::Default, 14.0, false);
				pLabel->SetForegroundColor(Color::SidePanelForeground);
				auto pControl = (*pField)->CreateControl(_pScrollPanel);
				pSizer->AddSpacer(8);
				pSizer->Add(pLabel, 0, SizerFlag::Expand | SizerFlag::Left, 4);
				pSizer->Add(pControl, 0, SizerFlag::Expand | SizerFlag::Top, 2);
			}
		}
		InvalidateLayout();
	}

	bool EditorScreen::OnKeyboardEvent(KeyboardEvent& event)
	{
		return false;
	}

}