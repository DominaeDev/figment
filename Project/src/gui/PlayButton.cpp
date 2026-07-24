#include <pch.h>
#include "gui/PlayButton.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	PlayButton::PlayButton(control_ptr pParent) : ThemedButton(pParent)
	{
		_pBackground = CreateControl<Image>(Resource::BACKGROUND_CIRCLE_48PX);
		_pBackground->SetSize(36, 36);

		_pIcon = _pBackground->CreateControl<Image>(Resource::ICON_PLAY);
		_pIcon->Center();

		SetSize(36, 36);
		OnButtonState();
		SetBackgroundColor(Color::Transparent);
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
			_pBackground->SetForegroundColor(fig::color { offColor, 0x40 });
			_pIcon->SetForegroundColor(Color::SidePanelForeground);
			break;
		case ButtonState::Hover:
			_pBackground->SetForegroundColor(fig::color { offColor, 0x80 });
			_pIcon->SetForegroundColor(Color::SidePanelForeground);
			break;
		case ButtonState::Pressed:
			_pBackground->SetForegroundColor(fig::color { offColor, 0xC0 });
			_pIcon->SetForegroundColor(Color::SidePanelForeground);
			break;
		case ButtonState::Disabled:
			_pBackground->SetForegroundColor(fig::color { 0xCCCCCC, 0x80 });
			_pIcon->SetForegroundColor(fig::color { 0x808080 });
			break;
		}
	}

	void PlayButton::SetIconState(IconState iconState)
	{
		_iconState = iconState;
		switch (iconState)
		{
		case IconState::Play:
			_pIcon->SetTexture(Resource::ICON_PLAY);
			_pIcon->Rotate(0.0);
			break;
		case IconState::Stop:
			_pIcon->SetTexture(Resource::ICON_STOP);
			_pIcon->Rotate(0.0);
			break;
		case IconState::Spinner:
			_pIcon->SetTexture(Resource::ICON_SPINNER);
			break;
		}
	}
}