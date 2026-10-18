#pragma once

namespace fig::gui
{
	class ITabControl
	{
	public:
		virtual void SetFocus(bool) = 0;
		virtual bool IsFocused() const = 0;
		virtual bool CanFocus() const = 0;
	};

	class TabControlGroup;
}