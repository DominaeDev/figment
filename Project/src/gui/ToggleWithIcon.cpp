#include <pch.h>
#include "gui/ToggleWithIcon.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	ToggleWithIcon::ToggleWithIcon(LayoutElement* pParent, TextureType icon, ToggleBehavior behavior, bool bOn) : ThemedButton(pParent),
		_bOn { bOn },
		_behavior { behavior }
	{
		_pBGRenderer = new TexturedBorderRenderer(TextureType::ROUNDED_BACKGROUND_6PX, 8);
		_pBGRenderer->SetColor(GetThemeBackground());
		SetBackgroundRenderer(_pBGRenderer);

		_pIcon = new Image(this, AppResources::GetTexture(icon));
		_pIcon->SetForegroundColor(GetThemeForeground());

		Toggle(bOn, false);

		SetSize(36, 36);

		switch (behavior)
		{
		default:
		case ToggleBehavior::Default:
			BaseButton::SetDelegate([this]() { Toggle(!this->_bOn); });
			break;
		case ToggleBehavior::Radio:
			BaseButton::SetDelegate([this]() { if (!this->_bOn) Toggle(true); });
			break;
		}
	}

	void ToggleWithIcon::SetDelegate(ToggleDelegate pDelegate) noexcept
	{
		_fnToggle = pDelegate;
	}

	void ToggleWithIcon::OnSize()
	{
		if (_pIcon)
			_pIcon->Center();
	}

	void ToggleWithIcon::OnButtonState()
	{
		_pBGRenderer->SetColor(GetThemeBackground());
		_pIcon->SetForegroundColor(GetThemeForeground());
	}

	void ToggleWithIcon::Toggle(bool bOn, bool bTrigger) noexcept
	{
		_bOn = bOn;
		if (bOn)
		{
			auto pBorder = new TexturedBorderRenderer(TextureType::ROUNDED_BORDER_6PX, 8);
			pBorder->SetColor(Colors::LineColor);
			SetBorderRenderer(pBorder);
		}
		else
			SetBorderRenderer(nullptr);

		if (bTrigger and _fnToggle)
			_fnToggle(_bOn);
	}
}