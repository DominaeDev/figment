#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class ImageViewport : public Control
	{
	public:
		ImageViewport(ControlPtr pParent, bool bClampZoom = true);
		
		void SetTexture(fig::texture_ptr pTexture) noexcept;
		void SetBackgroundTexture(fig::texture_ptr pBGTexture) noexcept;
		void SetMask(fig::texture_ptr pTexture, fig::coord cornerSize) noexcept;

		void ResetTransform();
		bool IsTransforming() const noexcept { return _bMouseDown; }

		fig::texture_ptr GetTargetTexture() noexcept { return _targetTexture.get(); }

	protected:
		void OnRender(fig::renderer_ptr pRenderer) override;
		EventResult OnEvent(fig::event& event);
		void OnSize() override;
		bool HandleMouseWheel(SDL_MouseWheelEvent& event);
		bool HandleMouseMotion(SDL_MouseMotionEvent& event);
		bool HandleMouseDown(SDL_MouseButtonEvent& event);
		bool HandleMouseUp(SDL_MouseButtonEvent& event);

		float GetFitScale(fig::coord width, fig::coord height) const;
		void ClampOffset(fig::point& offset, fig::coord width, fig::coord height);
		fig::rectf CalcDrawRect(fig::coord width, fig::coord height, float zoom, fig::point offset) const;
		void Redraw();
		void SetDirty();

	protected:
		bool _bRedraw = true;
		bool _bRedrawAlpha = true;
		
		fig::point _offset {};
		float _fZoom { 1.0f };
		fig::point _imageSize {};
		float _fImageRatio {};
		bool _bClampZoom { true };

		fig::point _mouseDownPos {};
		fig::point _mouseDownOffset {};
		bool _bMouseDown {};
		fig::point _lastSize {};

		fig::sdl::Texture _targetTexture;
		fig::texture_ptr _pTexture = nullptr;
		fig::texture_ptr _pBGTexture = nullptr;
		fig::texture_ptr _pMask = nullptr;
		fig::coord _cornerSize {};
	};
}