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
		fig::observer_ptr<TexturedBorderRenderer> _pBGRenderer;
		fig::observer_ptr<TexturedBorderRenderer> _pBorder;
		fig::observer_ptr<Image> _pIcon;
	};
}
