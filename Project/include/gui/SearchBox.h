#pragma once

#include "gui/TextBox.h"

namespace fig::gui
{
	class SearchBox : public TextBox
	{
	public:
		SearchBox(control_ptr pParent, FontFace fontFace, double ptSize);

	protected:
		void OnSize();

	private:
		fig::observer_ptr<Image> _pIcon;
	};
}