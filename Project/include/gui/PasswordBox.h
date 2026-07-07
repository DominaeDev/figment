#pragma once

#include "gui/SimpleTextBox.h"

namespace fig::gui
{
	class PasswordBox : public SimpleTextBox
	{
	public:
		PasswordBox(LayoutElement* pParent);

	protected:
		void OnSize() override;
		void OnEnabled(bool bEnabled) override;

	private:
		fig::observer_ptr<Image> _pIcon;
	};
}