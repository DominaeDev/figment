#pragma once

#include "Figment.h"
#include <array>

namespace fig::gui
{
	class NineGridImage : public Control
	{
	public:
		NineGridImage(LayoutElement* pParent, TexturePtr pTexture, Coord cornerPixels = 16);
		NineGridImage(LayoutElement* pParent, TexturePtr pTexture, Corners corners);

		void SetTexture(TexturePtr pTexture);
		void SetCornerSize(Coord cornerSize);

	protected:
		void OnRender(Renderer* pRenderer) override;
		void OnUpdate(float fElapsed) override {};

	protected:
		std::array<float, 4> _cornerPixels { 16.0f, 16.0f, 16.0f, 16.0f };
		float _cornerSize = 20.0f;
		Color _bgColor {};
		Color _borderColor {};
		TexturePtr _pTexture = nullptr;
	};
}
