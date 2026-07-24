#include <pch.h>
#include "gui/PasswordBox.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	PasswordBox::PasswordBox(control_ptr pParent) : SimpleTextBox(pParent, FontFace::Default, 18, { TextBox::Flag::Password, TextBox::Flag::Single })
	{
		SetMarginLeft(34);

		_pIcon = CreateControl<Image>(AppResources::GetTexture(Resource::ICON_LOCK));
		_pIcon->SetForegroundColor(Color::SidePanelForeground);
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
		SimpleTextBox::OnEnabled(bEnabled);

		_pIcon->SetForegroundColor(bEnabled ? Color::SidePanelForeground : Color::DisabledForeground);
	}
}