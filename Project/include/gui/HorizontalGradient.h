#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class HorizontalGradient : public Control
	{
	public:
		HorizontalGradient(ControlPtr pParent, fig::color colorLeft, fig::color colorRight);
		void SetColors(fig::color colorLeft, fig::color colorRight);

	protected:
		void OnUpdate(float fElapsed) override {};
		void OnRender(fig::renderer_ptr pRenderer) override;

		void RefreshGeometry(const fig::rect& rect);
	private:
		fig::color _colorLeft {};
		fig::color _colorRight {};
		fig::rect _lastRect {};
		fig::texture_ptr _pTexture;

		std::vector<fig::vertex> _vertices {};
	};
}