#pragma once

#include "gui/ButtonWithLabel.h"

namespace fig::gui
{
	class ToggleWithLabel : public ButtonWithLabel
	{
		ToggleWithLabel() = delete;
	public:
		ToggleWithLabel(ControlPtr pParent, fig::string_view text, double fontSize = 16.0, ToggleBehavior behavior = ToggleBehavior::Default, bool bOn = false);

		void SetDelegate(ToggleDelegate pDelegate) noexcept;
		void Toggle(bool bOn, bool bSilent = false) noexcept;
		bool IsOn() const noexcept { return _bOn; }
	protected:
		void OnButtonState() override;

	private:
		ToggleDelegate _fnToggle {};
		ToggleBehavior _behavior {};
		bool _bOn {};
	};
}
