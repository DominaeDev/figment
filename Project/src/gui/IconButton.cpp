#include <pch.h>
#include "gui/IconButton.h"
#include "gui/NineGridBackgroundRenderer.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	IconButton::IconButton(LayoutElement* pParent, TextureType icon) : Control(pParent), ButtonBase(this)
	{
		_pFaceRenderer = new NineGridBackgroundRenderer(8);
		_pFaceRenderer->SetTextures(AppResources::GetTexture(TextureType::BUTTON_BACKGROUND), nullptr);
		SetBackgroundRenderer(_pFaceRenderer);

		_pIcon = new Image(this, AppResources::GetTexture(icon), Colors::Black);
	}

	void IconButton::OnUpdate(float fElapsed)
	{
		switch (_state)
		{
		case State::Default:
			_pFaceRenderer->SetColor(Colors::Transparent);
			break;
		case State::Hover:
			_pFaceRenderer->SetColor(Color { 0xe4, 0xe0, 0xd1, 0xff });
			break;
		case State::Pressed:
			_pFaceRenderer->SetColor(Color { 0xdf, 0xd5, 0xc3, 0xff });
			break;
		case State::Disabled:
			_pFaceRenderer->SetColor(Color { 0xe1, 0xdf, 0xd8, 0xff });
			break;
		}
	}

	bool IconButton::OnEvent(Event& event)
	{
		return HandleMouseEvents(event);
	}

	void IconButton::OnSize()
	{
		if (_pIcon)
			_pIcon->Center();
	}
}