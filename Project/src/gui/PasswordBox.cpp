#include <pch.h>
#include "gui/PasswordBox.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	PasswordBox::PasswordBox(ControlPtr pParent) : TextBox(pParent, FontFace::Default, 18, Mode::Password)
	{
		SetMarginLeft(34);

		_pIcon = CreateControl<Image>(AppResources::GetTexture(Resource::ICON_LOCK));
		_pIcon->SetForegroundColor(Color::Icon);
	}

	void PasswordBox::OnSize()
	{
		if (_pIcon)
		{
			_pIcon->SetX(4);
			_pIcon->CenterVertically();
		}
	}

	void PasswordBox::OnEnabled(bool bEnabled)
	{
		TextBox::OnEnabled(bEnabled);

		_pIcon->SetForegroundColor(bEnabled ? Color::Icon : Color::DisabledForeground);
	}
}