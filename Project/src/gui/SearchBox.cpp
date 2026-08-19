#include <pch.h>
#include "gui/SearchBox.h"
#include "gui/AppResources.h"
#include "gui/CustomRenderers.h"
#include "gui/GUIUtility.h"

namespace fig::gui
{
	SearchBox::SearchBox(ControlPtr pParent) : TextBox(pParent, FontFace::Default, Constants::GUI::TextBoxFontSize, TextInput::Flags { TextInput::Flag::Single })
	{
		SetMarginLeft(30); // Icon
		SetMarginRight(30); // Cross

		_pIcon = CreateControl<Image>(AppResources::GetTexture(Resource::ICON_SEARCH));
		_pIcon->SetForegroundColor(Color::SidePanelForeground);

		_pCross = CreateControl<Image>(AppResources::GetTexture(Resource::ICON_CROSS));
		_pCross->SetForegroundColor(Color::SidePanelForeground);
		_pCross->SetVisible(false);

		auto pTextBoxBG = SetBackgroundRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BACKGROUND_6PX, 8);
		pTextBoxBG->SetExtend(0.0f);
		pTextBoxBG->SetColor(Color::White);

		auto pTextBoxBorder = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_6PX, 8);
		pTextBoxBorder->SetExtend(0.0f);
		pTextBoxBorder->SetColor(Color::LineColor);
	}

	void SearchBox::OnSize()
	{
		_pIcon->SetX(4);
		_pIcon->CenterVertically();

		_pCross->SetX(GetWidth() - 4 - _pCross->GetWidth());
		_pCross->CenterVertically();
	}

	EventResult SearchBox::OnEvent(fig::event& event)
	{
		if (HandleMouseEvents(event))
			return EventResult::Handled;
		
		return TextBox::OnEvent(event);
	}

	bool SearchBox::HandleMouseEvents(const fig::event& event) noexcept
	{
		if (not (_bEnabled or _pCross->GetVisible()))
			return false;

		auto& rect = _pCross->GetRect();
		fig::coord expand = 0;

		if (event.type == SDL_EVENT_MOUSE_MOTION)
		{
			auto motionEvent = event.motion;
			if (is_inside(rect, toI(motionEvent.x), toI(motionEvent.y), expand))
			{
				if (not _bMouseInside and not _bMouseDown)
					_bMouseInside = true;
			}
			else
			{
				if (_bMouseInside or _bMouseDown)
				{
					_bMouseInside = false;
					_bMouseDown = false;
				}
			}
			return false; // Don't consume event
		}

		if ((event.type == SDL_EVENT_MOUSE_BUTTON_DOWN or event.type == SDL_EVENT_MOUSE_BUTTON_UP) and event.button.button == SDL_BUTTON_LEFT)
		{
			auto mouseEvent = event.button;

			if (not is_inside(rect, toI(mouseEvent.x), toI(mouseEvent.y), expand))
				return false; // Ignore

			if (mouseEvent.down != _bMouseDown)
			{
				if (_bMouseDown and !mouseEvent.down)
				{
					// Clicked cross
					Clear();
				}

				_bMouseDown = mouseEvent.down;
				_bMouseInside = false;
				return true;
			}
		}

		return false;
	}

	void SearchBox::OnText(fig::string_view text) noexcept
	{
		_pCross->SetVisible(text.length() > 0);
	}
}