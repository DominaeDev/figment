#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class HorizontalGradient : public Control
	{
	public:
		HorizontalGradient(ControlPtr pParent, fig::color_ref colorLeft, fig::color_ref colorRight);
		void SetColors(fig::color_ref colorLeft, fig::color_ref colorRight);

	protected:
		void OnUpdate(float fElapsed) override {};
		void OnRender(fig::renderer_ptr pRenderer) override;

		void RefreshGeometry(const fig::rect& rect);
	private:
		fig::color_ref _colorLeft {};
		fig::color_ref _colorRight {};
		fig::rect _lastRect {};
		fig::texture_ptr _pTexture;

		std::vector<fig::vertex> _vertices {};
	};
}