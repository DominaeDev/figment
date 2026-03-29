#include <pch.h>
#include "gui/PasswordBox.h"

namespace fig::gui
{
	PasswordBox::PasswordBox(LayoutElement* pParent) : SimpleTextBox(pParent, FontFace::Default, 18, { TextBox::Flag::Password, TextBox::Flag::Single })
	{
	}
}