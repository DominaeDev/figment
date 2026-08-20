#pragma once

#include "Figment.h"
#include <functional>

namespace fig::gui
{
	using MouseClickedDelegate = std::function<void()>;
	using MouseEnterDelegate = std::function<void()>;
	using MouseExitDelegate = std::function<void()>;
	using MouseDownDelegate = std::function<void(fig::point)>;
	using MouseUpDelegate = std::function<void(fig::point)>;

	class MouseEventHandler
	{
		MouseEventHandler() = delete;
	public:
		void SetClickableRegion(const fig::rect& localRect, fig::coord expand = 0) noexcept;
		void SetExpandSize(fig::coord size) noexcept;

		void SetDelegate(MouseClickedDelegate pDelegate) noexcept;
		void SetMouseEnterDelegate(MouseEnterDelegate pDelegate) noexcept;
		void SetMouseExitDelegate(MouseExitDelegate pDelegate) noexcept;
		void SetMouseDownDelegate(MouseDownDelegate pDelegate) noexcept;
		void SetMouseUpDelegate(MouseUpDelegate pDelegate) noexcept;

	protected:
		MouseEventHandler(ControlPtr pControl);
		void DropState() noexcept;

		EventResult HandleMouseEvents(const fig::event& event) noexcept;
		void Enable(bool bEnable) noexcept;

		virtual void OnButtonState() {}
		virtual void OnMouseEnter() {}
		virtual void OnMouseExit() {}
		virtual void OnButtonDown() {}
		virtual void OnButtonUp() {}
		virtual void OnClicked() {}

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
		ControlPtr _pOwner {};
		bool _bMouseInside = false;
		bool _bMouseDown = false;
		bool _bEnabled = true;
		fig::rect _region {};
		fig::coord _expand = 0;

		MouseClickedDelegate _fnClicked {};
		MouseEnterDelegate _fnEnter {};
		MouseExitDelegate _fnExit {};
		MouseDownDelegate _fnDown {};
		MouseUpDelegate _fnUp {};
	};

}
