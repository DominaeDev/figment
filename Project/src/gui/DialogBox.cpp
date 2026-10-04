#include <pch.h>
#include "gui/DialogBox.h"
#include "gui/Image.h"
#include "gui/KeyboardMods.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/ButtonWithLabel.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	constexpr fig::coord MinDialogWidth = 340;
	constexpr fig::coord MinDialogHeight = 220;
	constexpr fig::coord MaxDialogWidth = 540;

	DialogBox::DialogBox(FramePtr pFrame, fig::string_view message, DialogButtons buttons, DialogBoxDelegate fnDelegate) : ModalOverlay(pFrame),
		_fnDelegate { fnDelegate }
	{
		BlurBackground();
		
		_pBox = CreateControl<Panel>();
		auto pBGRenderer = _pBox->SetBackgroundRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BACKGROUND_10PX, 16);
		pBGRenderer->SetColor(GetBackgroundColor());
		auto pBorder = _pBox->SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_10PX, 16);
		pBorder->SetColor(Color::Border);
		
		auto pSizer = new VerticalSizer();
		
		_pMessage = _pBox->CreateControl<StaticText>("", FontFace::Default, Constants::GUI::DefaultFontSize, false);
		_pMessage->SetForegroundColor(Color::AppForeground);
		_pMessage->SetAlignment(TextAlignment::MiddleCenter);
		_pMessage->EnableWordWrap(true);
		pSizer->Add(_pMessage, -1, SizerFlag::Expand | SizerFlag::AlignCenterHorizontal | SizerFlag::AlignCenterVertical);
		
		auto pBorderSizer = _pBox->SetSizer<VerticalSizer>();
		pBorderSizer->Add(pSizer, -1, SizerFlag::Fill | SizerFlag::All, 12);

		auto pButtonSizer = new HorizontalSizer();
		if (buttons.IsSet(DialogButton::Ok))
			CreateButton(pButtonSizer, DialogButton::Ok, "OK");
		if (buttons.IsSet(DialogButton::Yes))
			CreateButton(pButtonSizer, DialogButton::Yes, "Yes");
		if (buttons.IsSet(DialogButton::Confirm))
			CreateButton(pButtonSizer, DialogButton::Confirm, "Confirm");
		if (buttons.IsSet(DialogButton::No))
			CreateButton(pButtonSizer, DialogButton::No, "No");
		if (buttons.IsSet(DialogButton::Dismiss))
			CreateButton(pButtonSizer, DialogButton::Dismiss, "Dismiss");
		if (buttons.IsSet(DialogButton::Cancel))
			CreateButton(pButtonSizer, DialogButton::Cancel, "Cancel");
		pSizer->Add(pButtonSizer, 0, SizerFlag::FixedSize, 36);
		pSizer->AddSpacer(8);

		SetText(message);
	}

	void DialogBox::OnUpdate(float fElapsed)
	{
		ModalOverlay::OnUpdate(fElapsed);
	}

	void DialogBox::OnRender(fig::renderer_ptr pRenderer)
	{
		// Don't call Control::OnRender
	}

	void DialogBox::SetText(fig::string_view text)
	{
		constexpr fig::coord HPadding = 80;
		constexpr fig::coord VPadding = 44;
		
		auto size = _pMessage->MeasureText(text);
		_pMessage->SetMaxLineWidth(std::clamp(size.x, MinDialogWidth - HPadding, MaxDialogWidth - HPadding));
		_pMessage->SetTextAndResize(text);
		
		_pBox->SetSize(std::max(MinDialogWidth, _pMessage->GetWidth() + HPadding), std::max(MinDialogHeight, _pMessage->GetHeight() + VPadding));
		_pBox->Center();
	}

	void DialogBox::OnSize()
	{
		ModalOverlay::OnSize();

		_pBox->Center();
	}

	EventResult DialogBox::OnEvent(fig::event& event)
	{
		if (event.type == SDL_EVENT_KEY_DOWN or event.type == SDL_EVENT_KEY_UP)
		{
			SDL_KeyboardEvent& keyEvent = event.key;
			KeyboardMods mods { event };

			if (keyEvent.down and not keyEvent.repeat)
			{
				if (keyEvent.key == SDLK_ESCAPE and mods.None)
				{
					OnButton(DialogButton::Cancel);
					return EventResult::Handled;
				}
			}
		}

		return EventResult::Pass;
	}

	void DialogBox::CreateButton(SizerPtr pSizer, DialogButton button, fig::string_view label)
	{
		auto pButton = CreateControl<ButtonWithLabel>(label);
		pButton->SetDelegate([this, button]() { return OnButton(button); });
		pSizer->Add(pButton, 1, SizerFlag::Fill | SizerFlag::Left | SizerFlag::Right, 6);
	}

	void DialogBox::OnButton(DialogButton button)
	{
		if (_fnDelegate)
			_fnDelegate(button);
		Close();
	}
}