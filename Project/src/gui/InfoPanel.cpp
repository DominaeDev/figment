#include <pch.h>
#include "gui/InfoPanel.h"
#include "gui/AppResources.h"
#include "gui/LineBorderRenderer.h"
#include "gui/KeyboardMods.h"
#include "gui/ImageWithMask.h"
#include "gui/ImageViewport.h"
#include "gui/ResizeHandle.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/CharacterDetailsPanel.h"
#include "chat/ChatSession.h"

using namespace fig::chat;
using namespace fig::data;
using namespace fig::io;

namespace fig::gui
{
	InfoPanel::InfoPanel(ControlPtr pParent) : Panel(pParent)
	{
		SetWidth(Constants::GUI::InfoPanel::DefaultWidth);
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
		_pViewport->SetHeight(Constants::GUI::InfoPanel::DefaultImageSize);

		_pCharacterDetails = _pExpandedRoot->CreateControl<CharacterDetailsPanel>();
		
		_pBottomPanel = _pExpandedRoot->CreateControl<Panel>();
		_pBottomPanel->SetBorderRenderer<LineBorderRenderer>(Color::LineColor, Direction::North);
		_pBottomPanel->SetHeight(180);

		auto pMainSizer = _pExpandedRoot->SetSizer<VerticalSizer>();
		pMainSizer->Add(_pViewport, -1, SizerFlag::Expand | SizerFlag::Greedy | SizerFlag::All, 6);
		pMainSizer->Add(_pCharacterDetails, -1, SizerFlag::Fill | SizerFlag::Right | SizerFlag::Left | SizerFlag::Bottom, 6);
		pMainSizer->Add(_pBottomPanel, 0, SizerFlag::Expand);

		_pResizeHandle = CreateControl<ResizeHandle>(Direction::West);
		_pResizeHandle->SetDelegate([this](fig::coord size) { Resize(size); });
		_pResizeHandle->SetClickDelegate([this]() { _bExpanded ? Collapse() : Expand(); });

		_bExpanded = false;
		Expand();
	}

	EventResult InfoPanel::OnEvent(fig::event& event)
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
				Global::GetUserSettings().GetBool(UserSetting::Interface::Chat::InfoPanelCollapsed) ? Collapse() : Expand();

				if (_bExpanded)
				{
					auto size = Global::GetUserSettings().GetInt(UserSetting::Interface::Chat::InfoPanelWidth, Constants::GUI::InfoPanel::DefaultWidth);
					fig::coord closestWidth = *std::ranges::min_element(Constants::GUI::InfoPanel::Widths, {}, [size](fig::coord width) { return std::abs(width - size); });
					SetWidth(closestWidth);
					EventResult::Continue;
				}

				if (_pViewport)
					_pViewport->SetHeight(Global::GetUserSettings().GetInt(UserSetting::Interface::Chat::ImageSize));
			}
		}

		return EventResult::Pass;
	}

	void InfoPanel::Expand() noexcept
	{
		if (_bExpanded)
			return;
		_bExpanded = true;

		if (Global::IsSignedIn())
		{
			Global::GetUserSettings().SetBool(UserSetting::Interface::Chat::InfoPanelCollapsed, false);
			auto size = Global::GetUserSettings().GetInt(UserSetting::Interface::Chat::InfoPanelWidth, Constants::GUI::InfoPanel::DefaultWidth);
			fig::coord closestWidth = *std::ranges::min_element(Constants::GUI::InfoPanel::Widths, {}, [size](fig::coord width) { return std::abs(width - size); });
			SetWidth(closestWidth);
		}
		else
			SetWidth(Constants::GUI::InfoPanel::DefaultWidth);

		SetBackgroundColor(Color::SidePanelBackground);

		_pExpandedRoot->Cull(false);
		_pCollapsedRoot->Cull(true);

		auto pTopSizer = SetSizer<VerticalSizer>();
		pTopSizer->Add(_pExpandedRoot, -1, SizerFlag::Expand | SizerFlag::Fill);

		PushEvent(UserEvent::SidePanelResized);
	}

	void InfoPanel::Collapse() noexcept
	{
		if (not _bExpanded)
			return;
		_bExpanded = false;

		if (Global::IsSignedIn())
			Global::GetUserSettings().SetBool(UserSetting::Interface::Chat::InfoPanelCollapsed, true);

		SetWidth(42);
		SetBackgroundColor(Color::AppBackground);

		_pExpandedRoot->Cull(true);
		_pCollapsedRoot->Cull(false);

		auto pTopSizer = SetSizer<VerticalSizer>();
		pTopSizer->Add(_pCollapsedRoot, -1, SizerFlag::Expand | SizerFlag::Fill);

		PushEvent(UserEvent::SidePanelResized);
	}

	void InfoPanel::SetImage(const fig::uuid& assetId)
	{
		if (auto try_image = Global::GetUserContent().GetTexture(assetId, GetSDLRenderer()))
			_pViewport->SetTexture((*try_image).get());
	}

	void InfoPanel::ClearImage() noexcept
	{
		_pViewport->SetTexture(nullptr);
	}

	void InfoPanel::OnSize()
	{
		if (_pResizeHandle)
			_pResizeHandle->FillParent();
	}

	void InfoPanel::Resize(fig::coord size) noexcept
	{
		if (_bExpanded and size < 80)
			Collapse();
		else if (not _bExpanded and size > 200)
			Expand();
		
		if (_bExpanded)
		{
			fig::coord closestWidth = *std::ranges::min_element(Constants::GUI::InfoPanel::Widths, {}, [size](fig::coord width) { return std::abs(width - size); });

			if (GetWidth() != closestWidth)
			{
				SetWidth(closestWidth);
				Global::GetUserSettings().SetInt(UserSetting::Interface::Chat::InfoPanelWidth, closestWidth);
				PushEvent(UserEvent::SidePanelResized);
			}
		}
	}

	void InfoPanel::OnAfterLayout()
	{
		constexpr fig::coord kGradientSize = 8;
		_pGradient->SetSize(kGradientSize, GetHeight());

//		_pViewport->SetY(6);
//		_pBottomPanel->SetY(GetHeight() - _pBottomPanel->GetHeight());
	}

	void InfoPanel::SetSession(const ChatSession& session)
	{
		auto botId = session.GetCharacterIdOf(Role::Bot1);
		if (auto try_portrait = Global::GetUserContent().GetLargePortraitForCharacter(botId))
			SetImage((*try_portrait).id);
		else
			ClearImage();

		if (auto try_character = Global::GetUserContent().Get<Character>(botId))
		{
			_pCharacterDetails->SetCharacter(*try_character);
		}
	}
}