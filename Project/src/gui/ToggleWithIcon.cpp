#include <pch.h>
#include "gui/ToggleWithIcon.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	ToggleWithIcon::ToggleWithIcon(ControlPtr pParent, Resource icon, ToggleBehavior behavior, bool bOn) : ThemedButton(pParent),
		_bOn { bOn },
		_behavior { behavior }
	{
		auto pBGRenderer = SetBackgroundRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BACKGROUND_6PX, 8);
		pBGRenderer->SetColor(GetThemeBackground());

		_pIcon = CreateControl<Image>(AppResources::GetTexture(icon));
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

	void ToggleWithIcon::SetIcon(Resource icon)
	{
		_pIcon->SetTexture(AppResources::GetTexture(icon));
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
		GetBackgroundRenderer()->SetColor(GetThemeBackground());
		_pIcon->SetForegroundColor(GetThemeForeground());
	}

	void ToggleWithIcon::Toggle(bool bOn, bool bTrigger) noexcept
	{
		_bOn = bOn;
		if (bOn)
		{
			auto pBorder = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_6PX, 8);
			pBorder->SetColor(Color::LineColor);
		}
		else
			ClearBorderRenderer();

		if (bTrigger and _fnToggle)
			_fnToggle(_bOn);
	}
}