#pragma once

#include "Control.h"

namespace fig::gui
{
	class TexturedBorder : public Control
	{
	public:
		TexturedBorder(control_ptr pParent, fig::texture_ptr borderTexture, int cornerPixels = 64);
		TexturedBorder(control_ptr pParent, fig::texture_ptr borderTexture, std::array<float, 4> corners);
		TexturedBorder(control_ptr pParent, Resource borderTexture, int cornerPixels = 64);
		TexturedBorder(control_ptr pParent, Resource borderTexture, std::array<float, 4> corners);

		void SetCornerScale(float cornerScale);
		void SetColors(fig::color bgColor, fig::color borderColor);
		void SetTexture(fig::texture_ptr borderTexture);

	protected:
		void OnRender(fig::renderer_ptr pRenderer) override;

	private:
		std::array<float, 4> _cornerPixels = { 64, 64, 64, 64 };
		float _cornerScale = 1.0f;
		fig::color _bgColor {};
		fig::color _borderColor {};
		fig::texture_ptr _pBorderTexture;
	};
}
