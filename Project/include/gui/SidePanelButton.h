#ifndef SIDE_PANEL_BUTTON_H__
#define SIDE_PANEL_BUTTON_H__
#pragma once

#include "gui/Control.h"
#include "gui/ThemedButton.h"

namespace fig::gui
{
	class NineGridRenderer;

	class SidePanelButton : public ThemedButton
	{
		SidePanelButton() = delete;
	public:
		SidePanelButton(LayoutElement* pParent, TextureType icon, const fig::string& label);

	protected:
		void OnSize() override;
		void OnButtonState() override;

	private:
		NineGridRenderer* _pBGRenderer;
		Image* _pIcon {};
		StaticText* _pLabel {};
	};
}

#endif