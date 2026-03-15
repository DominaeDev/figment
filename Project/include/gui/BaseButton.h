#ifndef BUTTON_BASE_H__
#define BUTTON_BASE_H__
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
		void SetExpandedArea(Coord size) noexcept;
		inline bool IsEnabled() const noexcept;

	protected:
		BaseButton(LayoutElement* pOwner);
		bool HandleMouseEvents(const Event& event) noexcept;
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
		LayoutElement* _pOwner;
		bool _bMouseInside = false;
		bool _bMouseDown = false;
		Coord _expand = 0;
	};

}

#endif