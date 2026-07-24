#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	using ResizeHandleSizeCallback = std::function<void(fig::coord)>;
	using ResizeHandleClickCallback = std::function<void()>;

	class ResizeHandle : public Control
	{
	public:
		ResizeHandle(ControlPtr pParent, Direction direction);
	
		void Render(fig::renderer_ptr pRenderer) override;
		void SetDelegate(ResizeHandleSizeCallback fnDelegate) { _fnOnResize = fnDelegate; }
		void SetClickDelegate(ResizeHandleClickCallback fnDelegate) { _fnOnClick = fnDelegate; }

	protected:
		void OnUpdate(float fElapsed) override;
		EventResult OnEvent(fig::event& event) override;

		fig::rectf GetHandleRect() const noexcept;

	private:
		Direction _direction {};
		bool _bHovering { false };
		bool _bResizing { false };
		float _fAlpha { 0.0f };

		ResizeHandleSizeCallback _fnOnResize;
		ResizeHandleClickCallback _fnOnClick;
		fig::rect _prevRect;
		fig::coord _prevSize;
		fig::coord _currSize;
	};
}