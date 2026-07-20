#include <pch.h>
#include "gui/ChatSidePanel.h"
#include "gui/AppResources.h"
#include "gui/LineBorderRenderer.h"
#include "gui/KeyboardMods.h"
#include "gui/ImageWithMask.h"
#include "gui/ImageViewport.h"

namespace fig::gui
{
	ChatSidePanel::ChatSidePanel(ParentPtr pParent) : Panel(pParent)
	{
		SetWidth(Constants::GUI::ChatSidePanel::Width);
		SetBackgroundColor(fig::gui::Colors::SidePanelBackground);

		_pExpandedRoot = CreateControl<Area>();
		_pCollapsedRoot = CreateControl<Area>();

		auto pGradient = _pExpandedRoot->CreateControl<HorizontalGradient>(Colors::SidePanelGradient.WithAlpha(0.8f), Colors::SidePanelGradient.WithAlpha(0.0f));
		_pGradient = pGradient;

		_pCollapseButton = CreateControl<ButtonWithIcon>(TextureType::ICON_EXPAND_ARROW_LEFT);
		_pCollapseButton->SetTheme(Themes::SidePanelButtonStyle);
		_pCollapseButton->SetSize(36, 36);
		_pCollapseButton->SetX(GetWidth() - _pCollapseButton->GetWidth() - 4);
		_pCollapseButton->SetY((Constants::GUI::SidePanel::HeaderHeight - _pCollapseButton->GetHeight()) / 2);
		_pCollapseButton->SetDelegate([this]() {
			_bExpanded ? Collapse() : Expand();
		});

		//_pImage = _pExpandedRoot->CreateControl<TexturedBorder>(nullptr, nullptr);
		//_pImage->SetHeight(380);

		_pViewport = _pExpandedRoot->CreateControl<ImageViewport>(nullptr, AppResources::GetTexture(TextureType::MASK_CARD));
		_pViewport->SetMaxSize(-1, 600);

		auto pBottom = _pExpandedRoot->CreateControl<Panel>();
		pBottom->SetBorderRenderer<LineBorderRenderer>(Colors::LineColor, Direction::North);
		pBottom->SetSize(-1, 180);

		auto pMainSizer = _pExpandedRoot->SetSizer<VerticalSizer>();
		auto pTopSizer = pMainSizer->Add<VerticalSizer>(-1, Sizer::Fill | Sizer::FlexSize, 600);
		pTopSizer->Add(_pViewport, -1, Sizer::Fill | Sizer::All, 6);
		pMainSizer->AddStretchSpacer();
		pMainSizer->Add(pBottom, -1, Sizer::Fill | Sizer::FlexSize, pBottom->GetHeight());

		_bExpanded = false;
		Expand();
	}

	void ChatSidePanel::OnAfterLayout()
	{
		constexpr Coord kGradientSize = 8;
		_pGradient->SetSize(kGradientSize, GetHeight());
	}

	EventResult ChatSidePanel::OnEvent(Event& event)
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
		SetBackgroundColor(fig::gui::Colors::SidePanelBackground);

		_pCollapseButton->SetX(-_pCollapseButton->GetWidth() - 4);
		_pCollapseButton->SetIcon(TextureType::ICON_SIDEBAR);
		_pExpandedRoot->Cull(false);
		_pCollapsedRoot->Cull(true);

		auto pTopSizer = SetSizer<VerticalSizer>();
		pTopSizer->Add(_pExpandedRoot, -1, Sizer::Expand | Sizer::Fill);

		SetBorderRenderer<LineBorderRenderer>(Colors::LineColor, Direction::West);

		PushEvent(UserEvent::SidePanelExpanded);
	}

	void ChatSidePanel::Collapse() noexcept
	{
		if (not _bExpanded)
			return;
		_bExpanded = false;

		SetWidth(42);
		SetBackgroundColor(fig::gui::Colors::AppBackground);

		_pCollapseButton->CenterHorizontally();
		_pCollapseButton->SetIcon(TextureType::ICON_EXPAND_ARROW_LEFT);
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