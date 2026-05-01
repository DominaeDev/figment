#pragma once

#include "Control.h"

namespace fig::gui
{
	class Image : public Control
	{
	public:
		Image(LayoutElement* pParent, TexturePtr pTexture, Color tint = { 0xFF, 0xFF, 0xFF, 0xFF });
		Image(LayoutElement* pParent, TextureType texture, Color tint = { 0xFF, 0xFF, 0xFF, 0xFF });
		void SetTexture(TexturePtr pTexture, bool bResize = false);
		void SetTexture(TextureType texture, bool bResize = false);
		inline bool HasTexture() const noexcept { return _pTexture != nullptr; }
		Point GetTextureSize() const noexcept;

		inline void Rotate(double angle) { _angle = angle; }

	protected:
		void OnRender(Renderer* pRenderer) override;

	protected:
		Texture* _pTexture {};
		double _angle {};
	};
}