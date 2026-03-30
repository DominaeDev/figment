#pragma once

#include "Control.h"
#include "gui/GUITypes.h"

namespace fig::gui
{
	class ImageWithMask : public Control
	{
	public:
		ImageWithMask(LayoutElement* pParent, TexturePtr pTexture, TexturePtr pMask, Color tint = { 0xFF, 0xFF, 0xFF, 0xFF });
		void SetTexture(TexturePtr pTexture, TexturePtr pMask, bool bResize = false);
		Point GetTextureSize() const noexcept;
		void Reset();

	protected:
		void OnRender(Renderer* pRenderer) override;

	protected:
		fig::sdl::Texture _texture;
	};
}
