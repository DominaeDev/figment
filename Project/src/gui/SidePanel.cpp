#include <pch.h>
#include "gui/SidePanel.h"
#include "gui/SidePanelMain.h"
#include "gui/SidePanelEditor.h"
#include "gui/AppResources.h"
#include "gui/ResizeHandle.h"
#include "gui/Editor.h"
#include "gui/KeyboardMods.h"

using namespace fig::io;

namespace fig::gui
{
	SidePanel::SidePanel(ControlPtr pParent) : Control(pParent)
	{
		SetWidth(Constants::GUI::SidePanel::Width);
		SetBackgroundColor(Color::SidePanelBackground);
		
		_pGradient = CreateControl<HorizontalGradient>(Color::SidePanelGradient.WithAlpha(0.0f), Color::SidePanelGradient.WithAlpha(0.8f));

		_pResizeHandle = CreateControl<ResizeHandle>(Direction::East);
		_pResizeHandle->SetDelegate([this](fig::coord size) { Resize(size); });
		_pResizeHandle->SetClickDelegate([this]() { _bExpanded ? Collapse() : Expand(); });
		
		SetSizer<VerticalSizer>();
		
		SetMode(Mode::Main);
		_bExpanded = false;
		Expand();
	}

	void SidePanel::OnAfterLayout()
	{
		constexpr fig::coord kGradientSize = 8;
		_pGradient->SetX(GetWidth() - kGradientSize);
		_pGradient->SetSize(kGradientSize, GetHeight());
	}

	EventResult SidePanel::OnEvent(fig::event& event)
	{
		if (IsUserEvent(event, UserEvent::UserSignedIn))
		{
			Global::GetUserSettings().GetBool(UserSetting::Interface::SidePanelCollapsed) ? Collapse() : Expand();
			return EventResult::Continue;
		}
		else if (IsUserEvent(event, UserEvent::UserSignedOut))
		{
			return EventResult::Continue;
		}

		if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP)
		{
			SDL_KeyboardEvent& keyEvent = event.key;
			KeyboardMods mods { event };

			if (keyEvent.down and not keyEvent.repeat)
			{
				if (keyEvent.key == SDLK_TAB and mods.None)
				{
					_bExpanded ? Collapse() : Expand();
					return EventResult::Handled;
				}
			}
		}

		return EventResult::Pass;
	}

	void SidePanel::Expand() noexcept
	{
		if (_bExpanded)
			return;
		_bExpanded = true;

		SetWidth(Constants::GUI::SidePanel::Width);
		if (_pContent)
			_pContent->ShowExpanded();

		if (Global::IsSignedIn())
			Global::GetUserSettings().SetBool(UserSetting::Interface::SidePanelCollapsed, false);

		_pGradient->SetVisible(true);

		PushEvent(UserEvent::SidePanelResized);
	}

	void SidePanel::Collapse() noexcept
	{
		if (not _bExpanded)
			return;
		_bExpanded = false;

		SetWidth(42);
		if (_pContent)
			_pContent->ShowCollapsed();

		if (Global::IsSignedIn())
			Global::GetUserSettings().SetBool(UserSetting::Interface::SidePanelCollapsed, true);

		_pGradient->SetVisible(false);

		PushEvent(UserEvent::SidePanelResized);
	}

	void SidePanel::OnSize()
	{
		if (_pResizeHandle)
			_pResizeHandle->FillParent();
	}

	void SidePanel::Resize(fig::coord size) noexcept
	{
		if (_bExpanded and size < 120)
			Collapse();
		else if (not _bExpanded and size > 200)
			Expand();
	}

	void SidePanel::SetMode(Mode mode)
	{
		if (_mode == mode)
			return;

		switch (mode)
		{
		case Main:
			SetContent<SidePanelMain>();
			break;
		case Editor:
			SetContent<SidePanelEditor>();
			break;
		}

		_mode = mode;
	}

	void SidePanel::SetEditor(fig::observer_ptr<fig::gui::Editor> pEditor)
	{
		if (pEditor)
			SetMode(Mode::Editor);
		else
			SetMode(Mode::Main);
	}
}