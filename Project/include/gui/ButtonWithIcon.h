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
		ButtonWithIcon(ControlPtr pParent, Resource icon);
		void SetIcon(Resource icon);
		void EnableBorder(bool bEnable) noexcept;

	protected:
		void OnSize() override;
		void OnButtonState() override;

	private:
		fig::observer_ptr<Image> _pIcon;
	};
}
