#ifndef BUTTON_H__
#define BUTTON_H__
#pragma once

#include "gui/Control.h"
#include "gui/ThemedButton.h"

namespace fig::gui
{
	class NineGridRenderer;

	class ButtonWithIcon : public ThemedButton
	{
		ButtonWithIcon() = delete;
	public:
		ButtonWithIcon(LayoutElement* pParent, TextureType icon);

	protected:
		void OnSize() override;
		void OnButtonState() override;

	private:
		NineGridRenderer* _pBGRenderer;
		Image* _pIcon {};
	};
}

#endif