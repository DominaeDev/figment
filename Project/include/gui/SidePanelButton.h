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
		SidePanelButton(LayoutElement* pParent, TextureType icon, const fig::string& label);

	protected:
		void OnSize() override;
		void OnButtonState() override;

	private:
		CustomRenderer* _pBorder;
		Image* _pIcon {};
		StaticText* _pLabel {};
	};
}
