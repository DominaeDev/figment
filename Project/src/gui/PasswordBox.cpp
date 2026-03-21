#include <pch.h>
#include "gui/PasswordBox.h"

using namespace fig::gui::util;

namespace fig::gui
{
	PasswordBox::PasswordBox(LayoutElement* pParent) : SimpleTextBox(pParent, FontFace::Default, 18, { TextBox::Flag::Password, TextBox::Flag::Single })
	{
	}
}