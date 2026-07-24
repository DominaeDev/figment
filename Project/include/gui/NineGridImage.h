#pragma once

#include "Figment.h"
#include <array>

namespace fig::gui
{
	class NineGridImage : public Control
	{
	public:
		NineGridImage(ControlPtr pParent, fig::texture_ptr pTexture, fig::coord cornerPixels = 16);
		NineGridImage(ControlPtr pParent, fig::texture_ptr pTexture, fig::corners corners);

		void SetTexture(fig::texture_ptr pTexture);
		void SetCornerSize(fig::coord cornerSize);

	protected:
		void OnRender(fig::renderer_ptr pRenderer) override;
		void OnUpdate(float fElapsed) override {};

	protected:
		std::array<float, 4> _cornerPixels { 16.0f, 16.0f, 16.0f, 16.0f };
		float _cornerSize = 20.0f;
		fig::color _bgColor {};
		fig::color _borderColor {};
		fig::texture_ptr _pTexture = nullptr;
	};
}
