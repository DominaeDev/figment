#include <pch.h>
#include "gui/RecoveryCodeModal.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/ButtonWithLabel.h"
#include "gui/KeyboardMods.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	RecoveryCodeModal::RecoveryCodeModal(FramePtr pFrame, fig::string_view code) : ModalOverlay(pFrame)
	{
		BlurBackground();

		_pBox = CreateControl<Panel>();
		_pBox->SetSize(520, 280);
		_pBox->Center();

		auto pBGRenderer = _pBox->SetBackgroundRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BACKGROUND_10PX, 16);
		pBGRenderer->SetColor(GetBackgroundColor());
		auto pBorder = _pBox->SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_10PX, 16);
		pBorder->SetColor(Color::Border);

		auto pSizer = new VerticalSizer();
		pSizer->AddStretchSpacer();
		_pMessage = _pBox->CreateControl<StaticText>("Recovery code:", FontFace::Default, Constants::GUI::DefaultFontSize, false);
		_pMessage->SetForegroundColor(Color::AppForeground);
		_pMessage->SetAlignment(TextAlignment::MiddleCenter);
		_pMessage->EnableWordWrap(true);
		pSizer->Add(_pMessage, 0, SizerFlag::Expand | SizerFlag::AlignCenterHorizontal);
		pSizer->AddSpacer(8);

		auto pCodeArea = CreateControl<Area>();
		pCodeArea->SetForegroundColor(Color::AppForeground);
		auto pCodeSizer = pCodeArea->SetSizer<VerticalSizer>();
		pCodeArea->SetSize(472, 80);
		pSizer->Add(pCodeArea, 0, SizerFlag::AlignCenterHorizontal);

		CreateCodeRow(pCodeArea, pCodeSizer, code, 0uz);
		CreateCodeRow(pCodeArea, pCodeSizer, code, 16uz);

		pSizer->AddSpacer(24);

		auto hint = "Make sure you write this down and store it in a safe place.\nThis code will not be visible again.";

		auto pHint = _pBox->CreateControl<StaticText>(hint, FontFace::Default, Constants::GUI::DefaultFontSize, false);
		pHint->SetForegroundColor(Color::HintText);
		pHint->SetAlignment(TextAlignment::MiddleCenter);
		pHint->EnableWordWrap(true);
		pSizer->Add(pHint, 0, SizerFlag::Expand | SizerFlag::AlignCenterHorizontal);

		auto pBorderSizer = _pBox->SetSizer<VerticalSizer>();
		pBorderSizer->Add(pSizer, -1, SizerFlag::Fill | SizerFlag::All, 12);

		pSizer->AddStretchSpacer();

		auto pButton = CreateControl<ButtonWithLabel>("Close");
		pButton->SetDelegate([this]() { return Close(); });
		pSizer->Add(pButton, 0, SizerFlag::AlignCenterHorizontal, 6);
		pSizer->AddSpacer(8);
	}

	void RecoveryCodeModal::OnSize()
	{
		ModalOverlay::OnSize();

		_pBox->Center();
	}

	EventResult RecoveryCodeModal::OnEvent(fig::event& event)
	{
		if (event.type == SDL_EVENT_KEY_DOWN or event.type == SDL_EVENT_KEY_UP)
		{
			SDL_KeyboardEvent& keyEvent = event.key;
			KeyboardMods mods { event };

			if (keyEvent.down and not keyEvent.repeat)
			{
				if (keyEvent.key == SDLK_ESCAPE and mods.None)
				{
					Close();
					return EventResult::Handled;
				}
			}
		}

		return EventResult::Pass;
	}

	void RecoveryCodeModal::CreateCodeRow(ControlPtr pParent, SizerPtr pSizer, fig::string_view code, size_t index)
	{
		if (code.length() < index + 16uz)
			code = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";

		auto pHSizer = new HorizontalSizer();
		pSizer->Add(pHSizer, 0, SizerFlag::FixedSize, 40);

		for (size_t i = 0; i < 16uz; i += 2uz)
		{
			if (i == 8uz)
				pHSizer->AddSpacer(24);
			auto pLabel = pParent->CreateControl<StaticText>(code.substr(index + i, 2), FontFace::Bold, 24.0);

			pHSizer->Add(pLabel, 0, SizerFlag::AlignCenterHorizontal | SizerFlag::FixedSize, 56);
		}
	}
}