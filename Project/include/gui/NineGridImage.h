#ifndef NINE_GRID_IMAGE_H__
#define NINE_GRID_IMAGE_H__
#pragma once

#include "Types.h"
#include <array>

namespace fig::gui
{
	class NineGridImage : public Control
	{
	public:
		NineGridImage(LayoutElement* pParent, Texture* pTexture, float cornerPixels = 16.0f);
		NineGridImage(LayoutElement* pParent, Texture* pTexture, std::array<float, 4> corners);

		void SetTexture(Texture* pTexture);
		void SetCornerSize(float cornerSize);

	protected:
		void OnRender(Renderer* pRenderer) override;
		void OnUpdate(float fElapsed) override {};

	protected:
		std::array<float, 4> _cornerPixels { 16.0f, 16.0f, 16.0f, 16.0f };
		float _cornerSize = 20.0f;
		Color _bgColor {};
		Color _borderColor {};
		Texture* _pTexture = nullptr;
	};
}
#endif
