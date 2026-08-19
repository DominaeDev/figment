#pragma once

#include "gui/TextBox.h"

namespace fig::gui
{
	class PasswordBox : public TextBox
	{
	public:
		PasswordBox(ControlPtr pParent);

	protected:
		void OnSize() override;
		void OnEnabled(bool bEnabled) override;

	private:
		fig::observer_ptr<Image> _pIcon;
	};
}