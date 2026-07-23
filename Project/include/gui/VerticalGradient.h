#pragma once

#include "Control.h"

namespace fig::gui
{
	class VerticalGradient : public Control
	{
	public:
		VerticalGradient(ParentPtr pParent, fig::color colorTop, fig::color colorBottom);
		void SetColors(fig::color colorTop, fig::color colorBottom);
		void SetTexture(fig::texture_ptr pTexture);

		void OnRender(fig::renderer_ptr pRenderer) override;
	protected:
		void OnUpdate(float fElapsed) override {};

		void RefreshGeometry(fig::rectf rect);
	private:
		fig::colorf _colorTop {};
		fig::colorf _colorBottom {};
		fig::rectf _lastRect {};
		fig::texture_ptr _pTexture;

		std::vector<fig::vertex> _vertices {};
	};
}