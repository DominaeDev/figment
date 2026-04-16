#ifndef CARD_IMAGE_H__
#define CARD_IMAGE_H__
#pragma once

#include "Control.h"

namespace fig::gui
{
	class CardImage : public Control
	{
	public:
		CardImage(LayoutElement* pParent, TexturePtr pTexture, TexturePtr pMask = nullptr) noexcept;
		void SetTexture(TexturePtr pTexture, bool bResize = false) noexcept;
		void SetMask(TexturePtr pTexture) noexcept;
		void SetZoom(float value) noexcept;

	protected:
		void OnRender(Renderer* pRenderer) override;
		void RecreateTexture();

	protected:
		bool _bRedraw = true;
		bool _bRedrawAlpha = true;
		float _fZoom = 0.0f;
		fig::sdl::Texture _targetTexture;
		TexturePtr _pTexture = nullptr;
		TexturePtr _pMask = nullptr;
	};
}

#endif