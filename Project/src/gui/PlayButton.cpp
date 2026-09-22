#include <pch.h>
#include "gui/PlayButton.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	PlayButton::PlayButton(ControlPtr pParent) : ThemedButton(pParent)
	{
		_pBackground = CreateControl<Image>(Resource::BACKGROUND_CIRCLE_48PX);
		_pBackground->SetSize(36, 36);

		_pIcon = _pBackground->CreateControl<Image>(Resource::ICON_PLAY);
		_pIcon->Center();

		SetTheme(Theme::PlayButtonStyle);

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
		_pBackground->SetForegroundColor(GetThemeBackground());
		_pIcon->SetForegroundColor(GetThemeForeground());
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