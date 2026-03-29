#pragma once

#include "gui/SimpleTextBox.h"

namespace fig::gui
{
	class PasswordBox : public SimpleTextBox
	{
	public:
		PasswordBox(LayoutElement* pParent);

	protected:
		void OnSize();

	private:
		Image* _pIcon;
	};
}