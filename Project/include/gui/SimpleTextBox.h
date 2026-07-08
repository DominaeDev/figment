#pragma once

#include "gui/TextBox.h"

namespace fig::gui
{
	class SimpleTextBox : public TextBox
	{
	public:
		SimpleTextBox(ParentPtr pParent, FontFace fontFace, double ptSize, Flags flags = {});

	protected:
		void OnEnabled(bool bEnabled) override;
	};
}