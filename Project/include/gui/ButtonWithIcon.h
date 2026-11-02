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
		ButtonWithIcon(ControlPtr pParent, TextureResource icon, bool bBorder = false);
		void SetIcon(TextureResource icon);

	protected:
		void OnSize() override;
		void OnButtonState() override;

	private:
		fig::observer_ptr<Image> _pIcon;
		bool _bShowBorder { true };
	};
}
