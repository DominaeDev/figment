#pragma once

#include "Control.h"

namespace fig::gui
{
	class CardImage : public Control
	{
	public:
		CardImage(ParentPtr pParent, fig::texture_ptr pTexture, fig::texture_ptr pMask = nullptr) noexcept;
		void SetTexture(fig::texture_ptr pTexture, bool bResize = false) noexcept;
		void SetMask(fig::texture_ptr pTexture) noexcept;
		void SetZoom(float value) noexcept;

	protected:
		void OnRender(fig::renderer_ptr pRenderer) override;
		void OnSize() override;
		void Redraw();
		void SetDirty();

	protected:
		bool _bRedraw = true;
		bool _bRedrawAlpha = true;
		fig::point _lastSize {};

		float _fZoom = 0.0f;
		float _fZoomExpand = 18.0f; // pixels
		fig::sdl::Texture _targetTexture;
		fig::texture_ptr _pTexture = nullptr;
		fig::texture_ptr _pMask = nullptr;
	};
}
