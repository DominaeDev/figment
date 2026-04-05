#ifndef BUTTON_WITH_LABEL_H__
#define BUTTON_WITH_LABEL_H__
#pragma once

#include "gui/Control.h"
#include "gui/ThemedButton.h"

namespace fig::gui
{
	class TexturedBorderRenderer;
	class TexturedBorder;

	class ButtonWithLabel : public ThemedButton
	{
		ButtonWithLabel() = delete;
	public:
		ButtonWithLabel(LayoutElement* pParent, const fig::string& text);

	protected:
		void OnAfterLayout() override;
		void OnButtonState() override;

	private:
		TexturedBorderRenderer* _pBGRenderer;
		StaticText* _pLabel {};
		TexturedBorder* _pBorder {};
	};
}

#endif
