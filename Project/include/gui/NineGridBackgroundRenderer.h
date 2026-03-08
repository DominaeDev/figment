#pragma once

#include "Types.h"
#include "CustomRenderer.h"

namespace fig::gui
{
	class NineGridBackgroundRenderer : public CustomRenderer
	{
	public:
		NineGridBackgroundRenderer(int cornerPixels = 64);
		NineGridBackgroundRenderer(std::array<float, 4> corners);

		void Render(Renderer* pRenderer, const Rectf& rect) override;
		void SetCornerSize(float cornerSize);
		void SetColor(Color bgColor);
		void SetColors(Color bgColor, Color borderColor);
		void SetTexture(Texture* bgTexture);
		void SetTextures(Texture* bgTexture, Texture* borderTexture);

	private:
		std::array<float, 4> _cornerPixels = { 64, 64, 64, 64 };
		float _cornerSize = 20.0f;
		Color _bgColor {};
		Color _borderColor {};
		Texture* _pBGTexture = nullptr;
		Texture* _pBorderTexture = nullptr;
	};
}