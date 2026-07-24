#pragma once

#include "Control.h"

namespace fig::gui
{
	class ImageWithMask : public Control
	{
	public:
		ImageWithMask(control_ptr pParent, fig::texture_ptr pTexture, fig::texture_ptr pMask, fig::color tint = { 0xFF, 0xFF, 0xFF, 0xFF });
		void SetTexture(fig::texture_ptr pTexture, fig::texture_ptr pMask, bool bResize = false);
		fig::point GetTextureSize() const noexcept;
		void Reset();

	protected:
		void OnRender(fig::renderer_ptr pRenderer) override;

	protected:
		fig::sdl::Texture _texture;
	};
}
