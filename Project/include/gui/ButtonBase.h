#ifndef BUTTON_BASE_H__
#define BUTTON_BASE_H__
#pragma once

#include "gui/LayoutElement.h"
#include <functional>

namespace fig::gui
{
	using ButtonDelegate = std::function<void()>;

	class ButtonBase
	{
		ButtonBase() = delete;
	public:
		void SetDelegate(ButtonDelegate pDelegate) noexcept;
		void SetEnabled(bool bEnabled) noexcept;
		void SetExpandedArea(float size) noexcept;
		inline bool IsEnabled() const noexcept;

	protected:
		ButtonBase(LayoutElement* pOwner);
		bool HandleMouseEvents(const Event& event) noexcept;

		virtual void OnMouseEnter() {}
		virtual void OnMouseExit() {}
		virtual void OnButtonDown() {}
		virtual void OnButtonUp() {}

	protected:
		enum class State
		{
			Default,
			Pressed,
			Hover,
			Disabled,
		} _state {};

	private:
		ButtonDelegate _fn {};
		LayoutElement* _pOwner;
		bool _bMouseInside = false;
		bool _bMouseDown = false;
		float _fExpand = 0.0f;
	};

}

#endif