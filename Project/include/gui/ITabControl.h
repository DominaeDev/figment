#pragma once

namespace fig::gui
{
	using TabDirection = int32_t;
	constexpr TabDirection TabDirectionForward = 1;
	constexpr TabDirection TabDirectionBackward = -1;
	class ITabControl
	{
	public:
		virtual void SetFocus(bool, TabDirection) = 0;
		virtual bool IsFocused() const = 0;
		virtual bool CanFocus() const = 0;
		virtual bool FocusNext() { return false; }
		virtual bool FocusPrev() { return false; }
	};

	class TabControlGroup;
}