#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class ImageViewport : public Control
	{
	public:
		ImageViewport(ParentPtr pParent, fig::texture_ptr pTexture, fig::texture_ptr pMask = nullptr) noexcept;
		void SetTexture(fig::texture_ptr pTexture) noexcept;
		void SetMask(fig::texture_ptr pTexture) noexcept;

		void ResetTransform();

	protected:
		void OnRender(fig::renderer_ptr pRenderer) override;
		EventResult OnEvent(fig::event& event);
		void OnSize() override;
		bool HandleMouseWheel(SDL_MouseWheelEvent& event);
		bool HandleMouseMotion(SDL_MouseMotionEvent& event);
		bool HandleMouseDown(SDL_MouseButtonEvent& event);
		bool HandleMouseUp(SDL_MouseButtonEvent& event);

		float GetFitScale() const;
		fig::rectf CalcDrawRect(float zoom, fig::point offset) const;
		void ClampOffset();
		void Redraw();
		void SetDirty();

	protected:
		bool _bRedraw = true;
		bool _bRedrawAlpha = true;

		fig::point _offset {};
		float _fZoom { 1.0f };
		fig::point _imageSize {};
		float _fImageRatio {};

		fig::point _mouseDownPos {};
		fig::point _mouseDownOffset {};
		bool _bMouseDown {};
		fig::point _lastSize {};

		fig::sdl::Texture _targetTexture;
		fig::texture_ptr _pTexture = nullptr;
		fig::texture_ptr _pMask = nullptr;
	};
}