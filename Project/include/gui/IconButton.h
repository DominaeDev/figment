#ifndef BUTTON_H__
#define BUTTON_H__
#pragma once

#include "gui/Control.h"
#include "gui/ThemedButton.h"

namespace fig::gui
{
	class NineGridBackgroundRenderer;

	class IconButton : public ThemedButton
	{
		IconButton() = delete;
	public:
		IconButton(LayoutElement* pParent, TextureType icon);

	protected:
		void OnSize() override;
		void OnButtonState() override;

	private:
		NineGridBackgroundRenderer* _pFaceRenderer;
		Image* _pIcon {};
	};
}

#endif