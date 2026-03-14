#pragma once

#include "gui/TextBox.h"

namespace fig::gui
{
	class SearchBox : public TextBox
	{
	public:
		SearchBox(LayoutElement* pParent, FontFace fontFace, double ptSize);

	protected:
		void OnSize();

	private:
		Image* _pIcon;
	};
}