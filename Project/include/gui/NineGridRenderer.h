#pragma once

#include "Types.h"
#include "CustomRenderer.h"

namespace fig::gui
{
	class NineGridRenderer : public CustomRenderer
	{
	public:
		NineGridRenderer(int cornerPixels = 64);
		NineGridRenderer(std::array<float, 4> corners);

		void Render(Renderer* pRenderer, const Rectf& rect) override;
		void SetColor(Color bgColor);
		void SetTexture(Texture* bgTexture);
		void SetCornerSize(float cornerSize);
		void SetExtend(float size);

	private:
		std::array<float, 4> _cornerPixels = { 64, 64, 64, 64 };
		float _fCornerSize = 20.0f;
		float _fExtend = 0.0f;
		Color _bgColor {};
		Color _borderColor {};
		Texture* _pBGTexture = nullptr;
		Texture* _pBorderTexture = nullptr;
	};
}