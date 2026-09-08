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

	void SidePanelEditor::SetEditor(fig::observer_ptr<Editor> pEditor) noexcept
	{
		_pEditor = pEditor;
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

		if (_pEditor)
		{
			auto pNavigationSizer = SetSizer<VerticalSizer>();
			pNavigationSizer->AddSpacer(56);

			auto pages = _pEditor->GetPageDescriptors();
			for (auto& page : pages)
			{
				auto pNavButton = CreateControl<SidePanelButton>(page.iconLarge, page.label);
				pNavButton->SetDelegate([page] { PushEvent(UserEvent::SelectEditorPage, static_cast<int32_t>(page.pageIndex)); });
				
				pNavigationSizer->Add(pNavButton, 0, SizerFlag::Expand | SizerFlag::Right | SizerFlag::Left, 12);
				pNavigationSizer->AddSpacer(4);
			}
		}
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

		if (_pEditor)
		{
			auto pNavigationSizer = SetSizer<VerticalSizer>();
			pNavigationSizer->AddSpacer(62);

			auto pages = _pEditor->GetPageDescriptors();
			for (auto& page : pages)
			{
				auto pNavButton = CreateControl<ButtonWithIcon>(page.iconSmall, false);
				pNavButton->SetTheme(Theme::SidePanelButtonStyle);
				pNavButton->SetDelegate([page] { PushEvent(UserEvent::SelectEditorPage, static_cast<int32_t>(page.pageIndex)); });

				pNavigationSizer->Add(pNavButton, 0, SizerFlag::AlignCenterHorizontal);
				pNavigationSizer->AddSpacer(8);
			}
		}
	}

}