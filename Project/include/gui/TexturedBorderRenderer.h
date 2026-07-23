#pragma once

#include "Figment.h"
#include "CustomRenderer.h"

namespace fig::gui
{
	class TexturedBorderRenderer : public CustomRenderer
	{
	public:
		explicit TexturedBorderRenderer(Resource borderTexture, int32_t cornerPixels = 64);
		explicit TexturedBorderRenderer(Resource borderTexture, std::array<int32_t, 4> cornerPixels);
		explicit TexturedBorderRenderer(fig::texture_ptr borderTexture, int32_t cornerPixels = 64);
		explicit TexturedBorderRenderer(fig::texture_ptr borderTexture, std::array<int32_t, 4> cornerPixels);

		void Render(fig::renderer_ptr pRenderer, const fig::rectf& rect) override;
		void SetTexture(fig::texture_ptr borderTexture);
		void SetCornerScale(float scale);
		void SetExtend(float size);

	private:
		std::array<float, 4> _cornerPixels { 64, 64, 64, 64 };
		float _fCornerScale { 1.0f };
		float _fExtend { 0.0f };
		fig::texture_ptr _pTexture {};
	};
}