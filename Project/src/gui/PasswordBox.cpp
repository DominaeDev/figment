#include <pch.h>
#include "gui/PasswordBox.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	PasswordBox::PasswordBox(LayoutElement* pParent) : SimpleTextBox(pParent, FontFace::Default, 18, { TextBox::Flag::Password, TextBox::Flag::Single })
	{
		SetMarginLeft(34);

		_pIcon = CreateControl<Image>(AppResources::GetTexture(TextureType::ICON_LOCK));
		_pIcon->SetForegroundColor(Colors::SidePanelForeground);
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

		_pIcon->SetForegroundColor(bEnabled ? Colors::SidePanelForeground : Colors::DisabledForeground);
	}
}