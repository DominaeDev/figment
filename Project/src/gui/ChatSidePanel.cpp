#include <pch.h>
#include "gui/ChatSidePanel.h"
#include "gui/AppResources.h"
#include "gui/LineBorderRenderer.h"
#include "gui/KeyboardMods.h"
#include "gui/ImageWithMask.h"
#include "gui/ImageViewport.h"
#include "gui/ResizeHandle.h"

namespace fig::gui
{
	ChatSidePanel::ChatSidePanel(ControlPtr pParent) : Panel(pParent)
	{
		SetWidth(Constants::GUI::ChatSidePanel::DefaultWidth);
		SetBackgroundColor(Color::SidePanelBackground);

		_pExpandedRoot = CreateControl<Area>();
		_pCollapsedRoot = CreateControl<Area>();

		auto pGradient = _pExpandedRoot->CreateControl<HorizontalGradient>(Color::SidePanelGradient.WithAlpha(0.8f), Color::SidePanelGradient.WithAlpha(0.0f));
		_pGradient = pGradient;

		_pCollapseButton = _pCollapsedRoot->CreateControl<ButtonWithIcon>(Resource::ICON_EXPAND_ARROW_LEFT);
		_pCollapseButton->SetTheme(Theme::DefaultButtonStyle);
		_pCollapseButton->SetSize(36, 36);
		_pCollapseButton->SetX(3);
		_pCollapseButton->SetY((Constants::GUI::SidePanel::HeaderHeight - _pCollapseButton->GetHeight()) / 2);
		_pCollapseButton->SetDelegate([this]() { _bExpanded ? Collapse() : Expand(); });

		_pViewport = _pExpandedRoot->CreateControl<ImageViewport>(nullptr, AppResources::GetTexture(Resource::MASK_CARD));
		_pViewport->SetMaxSize(-1, 600);

		_pBottomPanel = _pExpandedRoot->CreateControl<Panel>();
		_pBottomPanel->SetBorderRenderer<LineBorderRenderer>(Color::LineColor, Direction::North);
		_pBottomPanel->SetSize(-1, 180);

		auto pMainSizer = _pExpandedRoot->SetSizer<VerticalSizer>();
		pMainSizer->Add(_pBottomPanel, 0, Sizer::Fill | Sizer::FixedSize, _pBottomPanel->GetHeight());
		pMainSizer->Add(_pViewport, -1, Sizer::Fill | Sizer::All, 6);

		_pResizeHandle = CreateControl<ResizeHandle>(Direction::West);
		_pResizeHandle->SetDelegate([this](fig::coord size) { Resize(size); });
		_pResizeHandle->SetClickDelegate([this]() { _bExpanded ? Collapse() : Expand(); });

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
		
		if (IsUserEvent(event, UserEvent::Activated))
		{
			if (Global::IsSignedIn())
			{
				Global::GetUserSettings().GetBool(UserSetting::ChatSidePanel_Collapsed) ? Collapse() : Expand();

				if (_bExpanded)
				{
					auto size = Global::GetUserSettings().GetInt(UserSetting::ChatSidePanel_Width, Constants::GUI::ChatSidePanel::DefaultWidth);
					fig::coord closestWidth = *std::ranges::min_element(Constants::GUI::ChatSidePanel::Widths, {}, [size](fig::coord width) { return std::abs(width - size); });
					SetWidth(closestWidth);
					EventResult::Continue;
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

		if (Global::IsSignedIn())
		{
			Global::GetUserSettings().SetBool(UserSetting::ChatSidePanel_Collapsed, false);
			auto size = Global::GetUserSettings().GetInt(UserSetting::ChatSidePanel_Width, Constants::GUI::ChatSidePanel::DefaultWidth);
			fig::coord closestWidth = *std::ranges::min_element(Constants::GUI::ChatSidePanel::Widths, {}, [size](fig::coord width) { return std::abs(width - size); });
			SetWidth(closestWidth);
		}
		else
			SetWidth(Constants::GUI::ChatSidePanel::DefaultWidth);

		SetBackgroundColor(Color::SidePanelBackground);

		_pExpandedRoot->Cull(false);
		_pCollapsedRoot->Cull(true);

		auto pTopSizer = SetSizer<VerticalSizer>();
		pTopSizer->Add(_pExpandedRoot, -1, Sizer::Expand | Sizer::Fill);

		PushEvent(UserEvent::SidePanelResized);
	}

	void ChatSidePanel::Collapse() noexcept
	{
		if (not _bExpanded)
			return;
		_bExpanded = false;

		if (Global::IsSignedIn())
			Global::GetUserSettings().SetBool(UserSetting::ChatSidePanel_Collapsed, true);

		SetWidth(42);
		SetBackgroundColor(Color::AppBackground);

		_pExpandedRoot->Cull(true);
		_pCollapsedRoot->Cull(false);

		auto pTopSizer = SetSizer<VerticalSizer>();
		pTopSizer->Add(_pCollapsedRoot, -1, Sizer::Expand | Sizer::Fill);

		PushEvent(UserEvent::SidePanelResized);
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

	void ChatSidePanel::OnSize()
	{
		if (_pResizeHandle)
			_pResizeHandle->FillParent();
	}

	void ChatSidePanel::Resize(fig::coord size) noexcept
	{
		if (_bExpanded and size < 80)
			Collapse();
		else if (not _bExpanded and size > 200)
			Expand();
		
		if (_bExpanded)
		{
			fig::coord closestWidth = *std::ranges::min_element(Constants::GUI::ChatSidePanel::Widths, {}, [size](fig::coord width) { return std::abs(width - size); });

			if (GetWidth() != closestWidth)
			{
				SetWidth(closestWidth);
				Global::GetUserSettings().SetInt(UserSetting::ChatSidePanel_Width, closestWidth);
				PushEvent(UserEvent::SidePanelResized);
			}
		}
	}

}