#include <pch.h>
#include "gui/PlayButton.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	PlayButton::PlayButton(ParentPtr pParent) : ThemedButton(pParent)
	{
		_pBackground = CreateControl<Image>(TextureType::BACKGROUND_CIRCLE_48PX);
		_pBackground->SetSize(36, 36);

		_pIcon = _pBackground->CreateControl<Image>(TextureType::ICON_PLAY);
		_pIcon->Center();

		SetSize(36, 36);
		OnButtonState();
		SetBackgroundColor(Colors::Transparent);
	}

	void PlayButton::OnUpdate(float fElapsed)
	{
		if (_iconState == IconState::Spinner)
		{
			_spinnerAngle += 135 * fElapsed;
			_pIcon->Rotate(_spinnerAngle);
		}
	}

	void PlayButton::OnSize()
	{
		if (_pIcon)
			_pIcon->Center();
	}

	void PlayButton::OnButtonState()
	{
		int32_t offColor { 0xa69982 };

		switch (_state)
		{
		case ButtonState::Default:
			_pBackground->SetForegroundColor(Color { offColor, 0x40 });
			_pIcon->SetForegroundColor(Colors::SidePanelForeground);
			break;
		case ButtonState::Hover:
			_pBackground->SetForegroundColor(Color { offColor, 0x80 });
			_pIcon->SetForegroundColor(Colors::SidePanelForeground);
			break;
		case ButtonState::Pressed:
			_pBackground->SetForegroundColor(Color { offColor, 0xC0 });
			_pIcon->SetForegroundColor(Colors::SidePanelForeground);
			break;
		case ButtonState::Disabled:
			_pBackground->SetForegroundColor(Color { 0xCCCCCC, 0x80 });
			_pIcon->SetForegroundColor(Color { 0x808080 });
			break;
		}
	}

	void PlayButton::SetIconState(IconState iconState)
	{
		_iconState = iconState;
		switch (iconState)
		{
		case IconState::Play:
			_pIcon->SetTexture(TextureType::ICON_PLAY);
			_pIcon->Rotate(0.0);
			break;
		case IconState::Stop:
			_pIcon->SetTexture(TextureType::ICON_STOP);
			_pIcon->Rotate(0.0);
			break;
		case IconState::Spinner:
			_pIcon->SetTexture(TextureType::ICON_SPINNER);
			break;
		}
	}
}