#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class ImageWithMask : public Control
	{
	public:
		ImageWithMask(ControlPtr pParent, fig::texture_ptr pTexture, fig::texture_ptr pMask, fig::color_ref tint = Color::White);
		
		void SetTexture(fig::texture_ptr pTexture, fig::texture_ptr pMask, bool bResize = false);
		fig::point GetTextureSize() const noexcept;
		bool HasTexture() const noexcept { return not _texture.empty(); }
		void Reset();

	protected:
		void OnRender(fig::renderer_ptr pRenderer) override;

	protected:
		fig::sdl::Texture _texture;
	};
}
