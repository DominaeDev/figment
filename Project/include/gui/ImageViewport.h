#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class ImageViewport : public Control
	{
	public:
		ImageViewport(ParentPtr pParent, TexturePtr pTexture, TexturePtr pMask = nullptr) noexcept;
		void SetTexture(TexturePtr pTexture) noexcept;
		void SetMask(TexturePtr pTexture) noexcept;

		void ResetTransform();

	protected:
		void OnRender(Renderer* pRenderer) override;
		EventResult OnEvent(Event& event);
		void OnSize() override;
		bool HandleMouseWheel(SDL_MouseWheelEvent& event);
		bool HandleMouseMotion(SDL_MouseMotionEvent& event);
		bool HandleMouseDown(SDL_MouseButtonEvent& event);
		bool HandleMouseUp(SDL_MouseButtonEvent& event);

		float GetFitScale() const;
		Rectf CalcDrawRect(float zoom, Point offset) const;
		void ClampOffset();
		void RecreateTexture();
		void SetDirty();

	protected:
		bool _bRedraw = true;
		bool _bRedrawAlpha = true;

		Point _offset {};
		float _fZoom { 1.0f };
		Point _imageSize {};
		float _fImageRatio {};

		Point _mouseDownPos {};
		Point _mouseDownOffset {};
		bool _bMouseDown {};
		Point _lastSize {};

		fig::sdl::Texture _targetTexture;
		TexturePtr _pTexture = nullptr;
		TexturePtr _pMask = nullptr;
	};
}