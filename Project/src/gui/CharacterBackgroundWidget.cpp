#include <pch.h>
#include "gui/CharacterBackgroundWidget.h"
#include "gui/PreviewCardImage.h"

namespace fig::gui
{
	static constexpr fig::coord Margin = 6;

	CharacterBackgroundWidget::CharacterBackgroundWidget(ControlPtr pParent) : Control(pParent), MouseEventHandler(this)
	{
		SetSize(Constants::GUI::CharacterEditor::BackgroundWidth + Margin * 2, Constants::GUI::CharacterEditor::BackgroundHeight + Margin * 2);

		_pImage = CreateControl<PreviewCardImage>(ImageFit::Outside);
		_pImage->SetPosition(Margin, Margin);
		_pImage->SetSize(Constants::GUI::CharacterEditor::BackgroundWidth, Constants::GUI::CharacterEditor::BackgroundHeight);
	}

	void CharacterBackgroundWidget::SetImage(const fig::uuid& assetId)
	{
		_pImage->SetImage(assetId);
	}

	void CharacterBackgroundWidget::SetImage(const fig::sdl::Surface& surface)
	{
		_pImage->SetImage(surface);
	}

	EventResult CharacterBackgroundWidget::OnEvent(fig::event& event)
	{
		return MouseEventHandler::HandleMouseEvents(event);
	}
}