#pragma once

#include "gui/SimpleTextBox.h"

namespace fig::gui
{
	class SearchBox : public SimpleTextBox
	{
	public:
		SearchBox(ControlPtr pParent);

	protected:
		void OnSize();

	private:
		fig::observer_ptr<Image> _pIcon;
	};
}