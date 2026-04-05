#pragma once

#include "Types.h"
#include "CustomRenderer.h"

namespace fig::gui
{
	class TexturedBorderRenderer : public CustomRenderer
	{
	public:
		explicit TexturedBorderRenderer(TextureType borderTexture, int32_t cornerPixels = 64);
		explicit TexturedBorderRenderer(TextureType borderTexture, std::array<int32_t, 4> cornerPixels);
		explicit TexturedBorderRenderer(TexturePtr borderTexture, int32_t cornerPixels = 64);
		explicit TexturedBorderRenderer(TexturePtr borderTexture, std::array<int32_t, 4> cornerPixels);

		void Render(Renderer* pRenderer, const Rectf& rect) override;
		void SetTexture(TexturePtr borderTexture);
		void SetCornerScale(float scale);
		void SetExtend(float size);

	private:
		std::array<float, 4> _cornerPixels { 64, 64, 64, 64 };
		float _fCornerScale { 1.0f };
		float _fExtend { 0.0f };
		TexturePtr _pTexture {};
	};
}