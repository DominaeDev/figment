#ifndef BUTTON_H__
#define BUTTON_H__
#pragma once

#include "gui/Control.h"
#include "gui/ButtonBase.h"

namespace fig::gui
{
	class NineGridBackgroundRenderer;

	class IconButton : public Control, public ButtonBase
	{
		IconButton() = delete;
	public:
		IconButton(LayoutElement* pParent, TextureType icon);

	protected:
		void OnUpdate(float fElapsed) override;
		bool OnEvent(Event& event) override;
		void OnSize() override;

	private:
		NineGridBackgroundRenderer* _pFaceRenderer;
		Image* _pIcon {};
	};
}

#endif