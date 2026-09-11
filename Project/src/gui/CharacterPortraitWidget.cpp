#include <pch.h>
#include "gui/CharacterPortraitWidget.h"
#include "gui/PreviewCardImage.h"
#include "gui/TexturedBorder.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	static constexpr fig::coord Margin = 6;
	static constexpr fig::coord Footer = 23;

	CharacterPortraitWidget::CharacterPortraitWidget(ControlPtr pParent) : Control(pParent), MouseEventHandler(this)
	{
		SetSize(Constants::GUI::CharacterEditor::PortraitWidth + Margin * 2, Constants::GUI::CharacterEditor::PortraitHeight + Margin * 2 + Footer);

		_pSelection = CreateControl<TexturedBorder>(AppResources::GetTexture(Resource::ROUNDED_BACKGROUND_10PX), 16);
		_pSelection->FillParent();
		_pSelection->SetForegroundColor(Color::StatusBarBackground);
		_pSelection->SetVisible(false);

		_pPortrait = CreateControl<PreviewCardImage>(ImageFit::Portrait);
		_pPortrait->SetPosition(Margin, Margin);

		_pLabel = CreateControl<StaticText>("Cover", FontFace::Default, Constants::GUI::DefaultFontSize, false);
		_pLabel->SetForegroundColor(Color::SidePanelForeground);
		_pLabel->SetAlignment(TextAlignment::MiddleTop);
		_pLabel->SetWidth(GetWidth() - Margin * 2);
		_pLabel->SetY(GetHeight() - (Footer + Margin) + 2);
		_pLabel->SetVisible(false);
	}

	void CharacterPortraitWidget::SetImage(const fig::uuid& assetId)
	{
		_pPortrait->SetImage(assetId);
	}

	void CharacterPortraitWidget::SetImage(const fig::sdl::Surface& surface)
	{
		_pPortrait->SetImage(surface);
	}

	void CharacterPortraitWidget::SetSelected(bool bSelected)
	{
		_pSelection->SetVisible(bSelected);
		_pLabel->SetBackgroundColor(bSelected ? Color::StatusBarBackground : GetBackgroundColor());
		_pLabel->SetVisible(bSelected);
	}

	EventResult CharacterPortraitWidget::OnEvent(fig::event& event)
	{
		return MouseEventHandler::HandleMouseEvents(event);
	}

	void CharacterPortraitWidget::OnSize()
	{
		if (_pLabel)
			_pLabel->CenterHorizontally();
	}
}