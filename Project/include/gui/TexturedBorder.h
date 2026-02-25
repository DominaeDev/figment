#ifndef TEXTURED_BORDER_H__
#define TEXTURED_BORDER_H__
#pragma once

#include "Control.h"

namespace fig::gui
{
	class TexturedBorder : public Control
	{
	public:
		TexturedBorder(Control* pParent, Texture* borderTexture, int cornerPixels = 64);
		TexturedBorder(Control* pParent, Texture* borderTexture, std::array<float, 4> corners);

		void SetCornerSize(float cornerSize);
		void SetColors(Color bgColor, Color borderColor);
		void SetTexture(Texture* borderTexture);

	protected:
		void OnRender(Renderer* pRenderer) override;

	private:
		std::array<float, 4> _cornerPixels = { 64, 64, 64, 64 };
		float _cornerSize = 20.0f;
		Color _bgColor {};
		Color _borderColor {};
		Texture* _pBorderTexture = nullptr;
	};
}

#endif