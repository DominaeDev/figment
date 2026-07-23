#pragma once

#include "gui/Control.h"
#include "gui/ThemedButton.h"

namespace fig::gui
{
	class TexturedBorderRenderer;

	class SidePanelButton : public ThemedButton
	{
		SidePanelButton() = delete;
	public:
		SidePanelButton(ParentPtr pParent, Resource icon, const fig::string& label);

	protected:
		void OnSize() override;
		void OnButtonState() override;

	private:
		fig::observer_ptr<CustomRenderer> _pBorder;
		fig::observer_ptr<Image> _pIcon {};
		fig::observer_ptr<StaticText> _pLabel {};
	};
}
