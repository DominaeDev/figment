#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	using ResizeHandleSizeDelegate = std::function<void(fig::coord)>;
	using ResizeHandleClickDelegate = std::function<void()>;

	class ResizeHandle : public Control
	{
	public:
		ResizeHandle(ControlPtr pParent, Direction direction);
	
		void Render(fig::renderer_ptr pRenderer) override;
		void SetDelegate(ResizeHandleSizeDelegate fnDelegate) { _fnOnResize = fnDelegate; }
		void SetClickDelegate(ResizeHandleClickDelegate fnDelegate) { _fnOnClick = fnDelegate; }
		void EnableDrawHandle(bool bEnable) { _bDrawHandle = bEnable; }

	protected:
		void OnUpdate(float fElapsed) override;
		EventResult OnEvent(fig::event& event) override;

		fig::rectf GetHandleRect() const noexcept;

	private:
		Direction _direction {};
		bool _bDrawHandle { true };
		bool _bHovering { false };
		bool _bMouseDown { false };
		fig::pointf _mouseDownPosition {};
		bool _bResizing { false };
		float _fAlpha { 0.0f };
		fig::pointf _lastClick {};
		ResizeHandleSizeDelegate _fnOnResize {};
		ResizeHandleClickDelegate _fnOnClick {};
		fig::rect _prevRect {};
		fig::coord _prevSize {};
		fig::coord _currSize {};
	};
}