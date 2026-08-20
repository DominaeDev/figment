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
		ButtonWithIcon(ControlPtr pParent, Resource icon, bool bBorder = false);
		void SetIcon(Resource icon);
		void ShowBorder(bool bEnable) noexcept;

	protected:
		void OnSize() override;
		void OnButtonState() override;

	private:
		fig::observer_ptr<Image> _pIcon;
		bool _bShowBorder { true };
	};
}
