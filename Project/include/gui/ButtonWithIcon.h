#ifndef BUTTON_WITH_ICON_H__
#define BUTTON_WITH_ICON_H__
#pragma once

#include "gui/Control.h"
#include "gui/ThemedButton.h"

namespace fig::gui
{
	class TexturedBorderRenderer;

	class ButtonWithIcon : public ThemedButton
	{
		ButtonWithIcon() = delete;
	public:
		ButtonWithIcon(LayoutElement* pParent, TextureType icon);
		void SetIcon(TextureType icon);
		void EnableBorder(bool bEnable) noexcept;

	protected:
		void OnSize() override;
		void OnButtonState() override;

	private:
		TexturedBorderRenderer* _pBGRenderer;
		TexturedBorderRenderer* _pBorder;
		Image* _pIcon {};
	};
}

#endif