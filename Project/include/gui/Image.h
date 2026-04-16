#pragma once

#include "Control.h"

namespace fig::gui
{
	class Image : public Control
	{
	public:
		Image(LayoutElement* pParent, Texture* pTexture, Color tint = { 0xFF, 0xFF, 0xFF, 0xFF });
		Image(LayoutElement* pParent, TextureType texture, Color tint = { 0xFF, 0xFF, 0xFF, 0xFF });
		void SetTexture(Texture* pTexture, bool bResize = false);
		inline bool HasTexture() const noexcept { return _pTexture != nullptr; }
		Point GetTextureSize() const noexcept;

	protected:
		void OnRender(Renderer* pRenderer) override;

	protected:
		Texture* _pTexture = nullptr;
	};
}