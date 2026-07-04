#pragma once

#include "Control.h"

namespace fig::gui
{
	class TexturedBorder : public Control
	{
	public:
		TexturedBorder(LayoutElement* pParent, Texture* borderTexture, int cornerPixels = 64);
		TexturedBorder(LayoutElement* pParent, Texture* borderTexture, std::array<float, 4> corners);
		TexturedBorder(LayoutElement* pParent, TextureType borderTexture, int cornerPixels = 64);
		TexturedBorder(LayoutElement* pParent, TextureType borderTexture, std::array<float, 4> corners);

		void SetCornerScale(float cornerScale);
		void SetColors(Color bgColor, Color borderColor);
		void SetTexture(Texture* borderTexture);

	protected:
		void OnRender(Renderer* pRenderer) override;

	private:
		std::array<float, 4> _cornerPixels = { 64, 64, 64, 64 };
		float _cornerScale = 1.0f;
		Color _bgColor {};
		Color _borderColor {};
		Texture* _pBorderTexture = nullptr;
	};
}
