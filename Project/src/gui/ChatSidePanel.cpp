#include <pch.h>
#include "gui/ChatSidePanel.h"
#include "gui/AppResources.h"
#include "gui/LineBorderRenderer.h"
#include "gui/KeyboardMods.h"
#include "gui/ImageWithMask.h"
#include "gui/ImageViewport.h"

namespace fig::gui
{
	ChatSidePanel::ChatSidePanel(control_ptr pParent) : Panel(pParent)
	{
		SetWidth(Constants::GUI::ChatSidePanel::Width);
		SetBackgroundColor(Color::SidePanelBackground);

		_pExpandedRoot = CreateControl<Area>();
		_pCollapsedRoot = CreateControl<Area>();

		auto pGradient = _pExpandedRoot->CreateControl<HorizontalGradient>(Color::SidePanelGradient.WithAlpha(0.8f), Color::SidePanelGradient.WithAlpha(0.0f));
		_pGradient = pGradient;

		_pCollapseButton = CreateControl<ButtonWithIcon>(Resource::ICON_EXPAND_ARROW_LEFT);
		_pCollapseButton->SetTheme(Theme::DefaultButtonStyle);
		_pCollapseButton->SetSize(36, 36);
		_pCollapseButton->SetX(GetWidth() - _pCollapseButton->GetWidth() - 4);
		_pCollapseButton->SetY((Constants::GUI::SidePanel::HeaderHeight - _pCollapseButton->GetHeight()) / 2);
		_pCollapseButton->SetDelegate([this]() {
			_bExpanded ? Collapse() : Expand();
		});

		_pViewport = _pExpandedRoot->CreateControl<ImageViewport>(nullptr, AppResources::GetTexture(Resource::MASK_CARD));
		_pViewport->SetMaxSize(-1, 600);

		_pBottomPanel = _pExpandedRoot->CreateControl<Panel>();
		_pBottomPanel->SetBorderRenderer<LineBorderRenderer>(Color::LineColor, Direction::North);
		_pBottomPanel->SetSize(-1, 180);

		auto pMainSizer = _pExpandedRoot->SetSizer<VerticalSizer>();
		pMainSizer->Add(_pBottomPanel, 0, Sizer::Fill | Sizer::FixedSize, _pBottomPanel->GetHeight());
		pMainSizer->Add(_pViewport, -1, Sizer::Fill | Sizer::All, 6);

		_bExpanded = false;
		Expand();
	}

	void ChatSidePanel::OnAfterLayout()
	{
		constexpr fig::coord kGradientSize = 8;
		_pGradient->SetSize(kGradientSize, GetHeight());

		_pViewport->SetY(6);
		_pBottomPanel->SetY(GetHeight() - _pBottomPanel->GetHeight());
	}

	EventResult ChatSidePanel::OnEvent(fig::event& event)
	{
		if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP)
		{
			SDL_KeyboardEvent& keyEvent = event.key;
			KeyboardMods mods { event };

			if (keyEvent.down and not keyEvent.repeat)
			{
				if (keyEvent.key == SDLK_TAB and mods.Control)
				{
					_bExpanded ? Collapse() : Expand();
					return EventResult::Handled;
				}
			}
		}

		return EventResult::Pass;
	}

	void ChatSidePanel::Expand() noexcept
	{
		if (_bExpanded)
			return;
		_bExpanded = true;

		SetWidth(Constants::GUI::ChatSidePanel::Width);
		SetBackgroundColor(Color::SidePanelBackground);

		_pCollapseButton->SetX(-_pCollapseButton->GetWidth() - 4);
		_pCollapseButton->SetIcon(Resource::ICON_SIDEBAR);
		_pExpandedRoot->Cull(false);
		_pCollapsedRoot->Cull(true);

		auto pTopSizer = SetSizer<VerticalSizer>();
		pTopSizer->Add(_pExpandedRoot, -1, Sizer::Expand | Sizer::Fill);

		SetBorderRenderer<LineBorderRenderer>(Color::LineColor, Direction::West);

		PushEvent(UserEvent::SidePanelExpanded);
	}

	void ChatSidePanel::Collapse() noexcept
	{
		if (not _bExpanded)
			return;
		_bExpanded = false;

		SetWidth(42);
		SetBackgroundColor(Color::AppBackground);

		_pCollapseButton->CenterHorizontally();
		_pCollapseButton->SetIcon(Resource::ICON_EXPAND_ARROW_LEFT);
		_pExpandedRoot->Cull(true);
		_pCollapsedRoot->Cull(false);

		auto pTopSizer = SetSizer<VerticalSizer>();
		pTopSizer->Add(_pCollapsedRoot, -1, Sizer::Expand | Sizer::Fill);

		SetBorderRenderer(nullptr);

		PushEvent(UserEvent::SidePanelCollapsed);
	}

	void ChatSidePanel::SetImage(const fig::uuid& assetId)
	{
		if (auto try_image = Global::GetUserContent().GetTexture(assetId, GetSDLRenderer()))
			_pViewport->SetTexture((*try_image).get());
	}

	void ChatSidePanel::ClearImage() noexcept
	{
		_pViewport->SetTexture(nullptr);
	}

}