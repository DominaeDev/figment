#include <pch.h>
#include "gui/ChatBackgroundPopOver.h"
#include "gui/BackgroundImageCarousel.h"
#include "gui/Frame.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/ImageAlignmentWidget.h"
#include "gui/ChatBackground.h"
#include "gui/BehindChat.h"
#include "gui/HorizontalLine.h"
#include "gui/Slider.h"
#include "gui/CheckBox.h"
#include "gui/ToggleWithLabel.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	constexpr fig::coord kSpacing = 6;
	constexpr fig::coord kMargin = 12;

	ChatBackgroundPopOver::ChatBackgroundPopOver(FramePtr pFrame, fig::observer_ptr<ChatBackground> pBackground, fig::observer_ptr<BehindChat> pBehindChat) : Overlay(pFrame),
		_pBackground(pBackground),
		_pBehindChat(pBehindChat)
	{
		SetSize(280, 480);
		SetBackgroundColor(Color::Transparent);
		SetForegroundColor(Color::PanelForeground);

		auto pBG = SetBackgroundRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BACKGROUND_10PX, 16);
		pBG->SetColor(fig::color_ref(Color::MenuBackground).WithAlpha(0.8f));

		auto pBorder = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_10PX, 16);
		pBorder->SetColor(Color::Border);

		auto pSizer = SetSizer<VerticalSizer>();

		pSizer->AddSpacer(kMargin);

		// Enabled
		_pEnabled = CreateControl<CheckBox>("Enable chat background");
		_pEnabled->SetValue(_pBackground->GetVisible(), true);
		_pEnabled->SetDelegate([this](bool bOn) { _pBackground->SetVisible(bOn); EnableControls(bOn); });
		pSizer->Add(_pEnabled, 0, SizerFlag::Left | SizerFlag::Right, kMargin);

		// ----
		pSizer->Add(CreateControl<HorizontalLine>(), 0, SizerFlag::Expand | SizerFlag::Left | SizerFlag::Right, kMargin);
		
		CreateLabel("Choose image");
		_pCarousel = CreateControl<BackgroundImageCarousel>();
		_pCarousel->SetHeight(64);
		_pCarousel->LoadCharacterPortraits(fig::uuid { "c44fe884-073e-4e6e-991c-0f8f875891de" }); //! @temp
		_pCarousel->SetDelegate([this](auto&& id) { SetImage(id); });
		pSizer->Add(_pCarousel, 0, SizerFlag::Expand | SizerFlag::Left | SizerFlag::Right, kMargin);
		pSizer->AddSpacer(kSpacing);

		// Scale fit
		CreateLabel("Crop fit");
		{
			auto pButtonArea = CreateControl<Area>();
			auto pButtonSizer = pButtonArea->SetSizer<HorizontalSizer>();

			_pFitInside = pButtonArea->CreateControl<ToggleWithLabel>("Inside");
			_pFitInside->SetOn(_pBackground->GetFit() == ImageFit::Inside, true);
			_pFitInside->SetSize(80, 25);
			_pFitInside->SetDelegate([this](bool bOn) { if (bOn) SetFit(ImageFit::Inside); });
			_pFitOutside = pButtonArea->CreateControl<ToggleWithLabel>("Outside");
			_pFitOutside->SetOn(_pBackground->GetFit() == ImageFit::Outside, true);
			_pFitOutside->SetSize(80, 25);
			_pFitOutside->SetDelegate([this](bool bOn) { if (bOn) SetFit(ImageFit::Outside); });

			pButtonSizer->Add(_pFitInside, 0);
			pButtonSizer->AddSpacer(kMargin);
			pButtonSizer->Add(_pFitOutside, 0);
			pButtonArea->LayoutNow();
			pButtonArea->ResizeToFit(true, true);
			pSizer->Add(pButtonArea, 0, SizerFlag::AlignCenterHorizontal);
			pSizer->AddSpacer(kSpacing);
		}

		// Bounds
		CreateLabel("Bounds");
		{
			auto pButtonArea = CreateControl<Area>();
			auto pButtonSizer = pButtonArea->SetSizer<HorizontalSizer>();

			_pFitArea = pButtonArea->CreateControl<ToggleWithLabel>("Chat");
			_pFitArea->SetOn(_pBackground->IsBounded(), true);
			_pFitArea->SetSize(80, 25);
			_pFitArea->SetDelegate([this](bool bOn) { SetBounded(bOn); });
			_pFitWindow = pButtonArea->CreateControl<ToggleWithLabel>("Window");
			_pFitWindow->SetOn(!_pBackground->IsBounded(), true);
			_pFitWindow->SetSize(80, 25);
			_pFitWindow->SetDelegate([this](bool bOn) { SetBounded(!bOn); });

			pButtonSizer->Add(_pFitArea, 0);
			pButtonSizer->AddSpacer(kMargin);
			pButtonSizer->Add(_pFitWindow, 0);
			pButtonArea->LayoutNow();
			pButtonArea->ResizeToFit(true, true);
			pSizer->Add(pButtonArea, 0, SizerFlag::AlignCenterHorizontal);
			pSizer->AddSpacer(kSpacing);
		}

		// Alignment
		CreateLabel("Alignment");
		_pAlignment = CreateControl<ImageAlignmentWidget>();
		_pAlignment->SetAlignment(_pBackground->GetAlignment(), true);
		_pAlignment->SetSize(80, 68);
		_pAlignment->SetDelegate([this](auto&& align) { _pBackground->SetAlignment(align); });
		pSizer->Add(_pAlignment, 0, SizerFlag::AlignCenterHorizontal);
		pSizer->AddSpacer(kSpacing);

		// ----
		pSizer->Add(CreateControl<HorizontalLine>(), 0, SizerFlag::Expand | SizerFlag::Left | SizerFlag::Right, kMargin);

		// Opacity
		CreateLabel("Opacity");
		_pOpacitySlider = CreateSlider();
		_pOpacitySlider->SetValue(_pBackground->GetAlpha(), true);
		_pOpacitySlider->SetDelegate([&](float value) {
			_pBackground->SetAlpha(value);
		});

		// Saturation
		CreateLabel("Saturation");
		_pSaturationSlider = CreateSlider();
		_pSaturationSlider->SetValue(_pBackground->GetSaturation(), true);
		_pSaturationSlider->SetDelegate([&](float value) {
			_pBackground->SetSaturation(value);
		});

		// Blur
		CreateLabel("Blur");
		_pBlurSlider = CreateSlider();
		_pBlurSlider->SetValue((_pBackground->GetBlur() - 1.0f) / 10.0f + 0.05f, true);
		_pBlurSlider->SetDelegate([&](float value) {
			int32_t blur = toI(value * 10.0f);
			if (blur > 0)
				_pBackground->SetBlur(toF(blur + 1));
			else
				_pBackground->SetBlur(0.0f); // Off
		});

		// Veil
		CreateLabel("Veil");
		_pVeilSlider = CreateSlider();
		_pVeilSlider->SetValue(_pBehindChat->GetAlpha(), true);
		_pVeilSlider->SetDelegate([&](float value) {
			_pBehindChat->SetAlpha(value);
		});

		// Zoom
		CreateLabel("Zoom");
		_pZoomSlider = CreateSlider();
		_pZoomSlider->SetRange(0.0f, 2.0f);
		_pZoomSlider->SetValue(_pBackground->GetZoom(), true);
		_pZoomSlider->SetDelegate([&](float value) {
			_pBackground->SetZoom(std::roundf(value * 20.0f) / 20.0f);
		});

		LayoutNow();
		ResizeToFit(false, true);

		SetHeight(GetHeight() + kSpacing + kMargin);

		SetVisible(false);
	}

	uint32_t ChatBackgroundPopOver::Show(fig::rect parentRect)
	{
		if (_pOwner)
			_pOwner->PopAllOverlays();

		auto menuY = parentRect.y + parentRect.h;
		auto menuHeight = GetHeight();
		auto maxY = _pOwner->GetHeight();
		if (menuHeight >= maxY)
			menuY = 0;
		else if (menuY + menuHeight > maxY)
			menuY = parentRect.y - menuHeight;
		menuY = std::max(menuY, 0);

		SetAbsolutePosition(fig::point { parentRect.x, menuY });
		SetVisible(true);

		return _pOwner->PushOverlay(this);
	}

	uint32_t ChatBackgroundPopOver::Show(fig::point position)
	{
		if (_pOwner)
			_pOwner->PopAllOverlays();

		if (position.x == -1 and position.y == -1)
		{
			float mx, my;
			SDL_GetMouseState(&mx, &my);
			position = fig::point { toI(mx), toI(my) };
		}

		SetAbsolutePosition(position);
		SetVisible(true);

		return _pOwner->PushOverlay(this);
	}

	EventResult ChatBackgroundPopOver::OnEvent(fig::event& event)
	{
		if (IsUserEvent(event, UserEvent::Scrolling))
		{
			if (_pOwner)
				_pOwner->PopOverlay(this);
			return EventResult::Continue;
		}
		return EventResult::Pass;
	}

	fig::observer_ptr<StaticText> ChatBackgroundPopOver::CreateLabel(fig::string_view text)
	{
		auto pSizer = GetSizer();
		auto pLabel = CreateControl<StaticText>(fig::string { text }, FontFace::Default, 14.0, false);
		pSizer->Add(pLabel, 0, SizerFlag::Expand | SizerFlag::Left | SizerFlag::Right, kMargin);
		pSizer->AddSpacer(kSpacing);
		return pLabel;
	}

	fig::observer_ptr<Slider> ChatBackgroundPopOver::CreateSlider()
	{
		auto pSizer = GetSizer();
		auto pSlider = CreateControl<Slider>();
		pSizer->Add(pSlider, 0, SizerFlag::Expand | SizerFlag::Left | SizerFlag::Right, kMargin);
		pSizer->AddSpacer(kSpacing + 6);
		return pSlider;
	}

	void ChatBackgroundPopOver::EnableControls(bool bEnabled)
	{
		_pOpacitySlider->SetEnabled(bEnabled);
		_pSaturationSlider->SetEnabled(bEnabled);
		_pZoomSlider->SetEnabled(bEnabled);
		_pBlurSlider->SetEnabled(bEnabled);
		_pVeilSlider->SetEnabled(bEnabled);
		_pCarousel->SetEnabled(bEnabled);
		_pFitInside->SetEnabled(bEnabled);
		_pFitOutside->SetEnabled(bEnabled);
		_pFitArea->SetEnabled(bEnabled);
		_pFitWindow->SetEnabled(bEnabled);
		_pAlignment->SetEnabled(bEnabled);
	}

	void ChatBackgroundPopOver::SetFit(ImageFit fit)
	{
		_pBackground->SetFit(fit);
		switch (fit)
		{
		case ImageFit::Inside:
			_pFitInside->SetOn(true, true);
			_pFitOutside->SetOn(false, true);
			break;
		case ImageFit::Outside:
			_pFitInside->SetOn(false, true);
			_pFitOutside->SetOn(true, true);
			break;
		}
	}

	void ChatBackgroundPopOver::SetBounded(bool bBounded)
	{
		_pBackground->SetBounded(bBounded);
		_pFitArea->SetOn(bBounded, true);
		_pFitWindow->SetOn(!bBounded, true);
	}

	void ChatBackgroundPopOver::SetImage(const fig::uuid& assetId)
	{
		_pBackground->SetImage(assetId);
//		_pBackground->SetZoom(0.0f);
//		_pZoomSlider->SetValue(0.0f, true);
	}
}