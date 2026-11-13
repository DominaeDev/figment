#include <pch.h>
#include "gui/TextInputDialogBox.h"
#include "gui/KeyboardMods.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/ButtonWithLabel.h"
#include "gui/AppResources.h"
#include "gui/FillParentSizer.h"
#include "gui/TextBox.h"

namespace fig::gui
{
	TextInputDialogBox::TextInputDialogBox(FramePtr pFrame, fig::string_view message, TextInputDialogDelegate fnDelegate) : ModalOverlay(pFrame),
		_fnDelegate { fnDelegate }
	{
		BlurBackground();
		
		_pBox = CreateControl<Panel>();
		auto pBGRenderer = _pBox->SetBackgroundRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BACKGROUND_10PX, 16);
		pBGRenderer->SetColor(GetBackgroundColor());
		auto pBorder = _pBox->SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_10PX, 16);
		pBorder->SetColor(Color::Border);
		
		auto pSizer = _pBox->SetSizer<FillParentSizer>();
		auto pInnerArea = _pBox->CreateControl<Area>();
		auto pInnerSizer = pInnerArea->SetSizer<VerticalSizer>();

		pSizer->Add(pInnerArea, -1, SizerFlag::All, 18);

		_pMessage = _pBox->CreateControl<StaticText>("", FontFace::Default, Constants::GUI::DefaultFontSize, false);
		_pMessage->SetForegroundColor(Color::AppForeground);
		_pMessage->EnableWordWrap(true);

		_pInput = _pBox->CreateControl<TextBox>();
		_pInput->SetFocus(true);
		_pInput->SetEnterPressedDelegate([this] (auto&&) { OnResult(true); });
		_pInput->SetEscapePressedDelegate([this] { OnResult(false); });
		_pInput->SetTextChangedDelegate([this](auto&& value) { _pButtonOk->SetEnabled(not empty_or_whitespace(value)); });
		
		pInnerSizer->Add(_pMessage, 0, SizerFlag::Expand | SizerFlag::Bottom, 8);
		pInnerSizer->Add(_pInput, 0, SizerFlag::Expand);
		pInnerSizer->AddSpacer(12);
		pInnerSizer->AddStretchSpacer();

		auto pButtonArea = _pBox->CreateControl<Area>();
		pButtonArea->SetHeight(36);
		auto pButtonSizer = pButtonArea->SetSizer<HorizontalSizer>();

		_pButtonOk = CreateControl<ButtonWithLabel>("OK");
		_pButtonOk->SetDelegate([this]() { return OnResult(true); });
		pButtonSizer->Add(_pButtonOk, 1, SizerFlag::Fill | SizerFlag::Right, 6);

		auto pButtonCancel = CreateControl<ButtonWithLabel>("Cancel");
		pButtonCancel->SetDelegate([this]() { return OnResult(false); });
		pButtonSizer->Add(pButtonCancel, 1, SizerFlag::Fill | SizerFlag::Left, 6);

		pInnerSizer->Add(pButtonArea, 0, SizerFlag::Expand);

		SetMessage(message);
	}

	void TextInputDialogBox::SetDefaultValue(fig::string_view text)
	{
		_pInput->SetText(text);
		_pInput->SelectAll();
		_pButtonOk->SetEnabled(not empty_or_whitespace(text));
	}

	void TextInputDialogBox::SetPlaceholder(fig::string_view text)
	{
		_pInput->SetPlaceholder(text);
	}

	void TextInputDialogBox::SetMaxLength(size_t length)
	{
		_pInput->SetMaxLength(length);
	}

	void TextInputDialogBox::OnUpdate(float fElapsed)
	{
		ModalOverlay::OnUpdate(fElapsed);
	}

	void TextInputDialogBox::SetMessage(fig::string_view text)
	{
		constexpr fig::coord DialogWidth = 460;
		constexpr fig::coord MinDialogHeight = 10;//220;
		constexpr fig::coord HPadding = 80;
		constexpr fig::coord VPadding = 96 + 12 * 2;
		
		_pMessage->SetTextWrapWidth(DialogWidth - HPadding);
		_pMessage->SetTextAndResize(text);
		
		_pBox->SetSize(DialogWidth, std::max(MinDialogHeight, _pMessage->GetHeight() + VPadding));
		_pBox->Center();
	}

	void TextInputDialogBox::OnSize()
	{
		_pBox->Center();
		ModalOverlay::OnSize();
	}

	EventResult TextInputDialogBox::OnEvent(fig::event& event)
	{
		if (event.type == SDL_EVENT_KEY_DOWN or event.type == SDL_EVENT_KEY_UP)
		{
			SDL_KeyboardEvent& keyEvent = event.key;
			KeyboardMods mods { event };

			if (keyEvent.down and not keyEvent.repeat)
			{
				if (keyEvent.key == SDLK_ESCAPE and mods.None)
				{
					OnResult(false);
					return EventResult::Handled;
				}
			}
		}

		return EventResult::Pass;
	}
	
	fig::observer_ptr<ButtonWithLabel> TextInputDialogBox::CreateButton(SizerPtr pSizer, fig::string_view label)
	{
		auto pButton = CreateControl<ButtonWithLabel>(label);
		pSizer->Add(pButton, 1, SizerFlag::Fill | SizerFlag::Left | SizerFlag::Right, 6);
		return pButton;
	}

	void TextInputDialogBox::OnResult(bool bConfirm)
	{
		auto result = _pInput->GetText();
		if (bConfirm and empty_or_whitespace(result))
			return; // Rejected

		if (_fnDelegate)
			_fnDelegate(bConfirm ? result : fig::string_view {});
		Close();
	}
}