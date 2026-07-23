#pragma once

#include "gui/LayoutElement.h"
#include <functional>

namespace fig::gui
{
	using ButtonDelegate = std::function<void()>;

	class BaseButton
	{
		BaseButton() = delete;
	public:
		void SetDelegate(ButtonDelegate pDelegate) noexcept;
		void SetEnabled(bool bEnabled) noexcept;
		void SetExpandedArea(fig::coord size) noexcept;
		inline bool IsEnabled() const noexcept;

	protected:
		BaseButton(ControlPtr pOwner);
		bool HandleMouseEvents(const fig::event& event) noexcept;
		void DropState() noexcept;

		virtual void OnButtonState() {}
		virtual void OnMouseEnter() {}
		virtual void OnMouseExit() {}
		virtual void OnButtonDown() {}
		virtual void OnButtonUp() {}

	protected:
		enum class ButtonState
		{
			Default,
			Pressed,
			Hover,
			Disabled,
		} _state {};
		void SetButtonState(ButtonState state);

	private:
		ButtonDelegate _fn {};
		fig::observer_ptr<LayoutElement> _pOwner;
		bool _bMouseInside = false;
		bool _bMouseDown = false;
		fig::coord _expand = 0;
	};

}
