#pragma once

#include "gui/Control.h"
#include "gui/ThemedButton.h"

namespace fig::gui
{
	class TexturedBorderRenderer;

	class ToggleWithIcon : public ThemedButton
	{
		ToggleWithIcon() = delete;
	public:
		ToggleWithIcon(ControlPtr pParent, Resource icon, ToggleBehavior behavior = ToggleBehavior::Default, bool bOn = false);

		void SetIcon(Resource icon);
		void SetDelegate(ToggleDelegate pDelegate) noexcept;
		void Toggle(bool bOn, bool bSilent = false) noexcept;
		bool IsOn() const noexcept { return _bOn; }

	protected:
		void OnSize() override;
		void OnButtonState() override;

	private:
		fig::observer_ptr<Image> _pIcon {};
		ToggleDelegate _fnToggle {};
		ToggleBehavior _behavior {};
		bool _bOn {};
	};
}
