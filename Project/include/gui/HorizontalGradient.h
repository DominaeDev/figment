#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class HorizontalGradient : public Control
	{
	public:
		HorizontalGradient(ControlPtr pParent, fig::color_ref_with_alpha colorLeft, fig::color_ref_with_alpha colorRight);
		void SetColors(fig::color_ref_with_alpha colorLeft, fig::color_ref_with_alpha colorRight);

	protected:
		void OnUpdate(float fElapsed) override {};
		void OnRender(fig::renderer_ptr pRenderer) override;

		void RefreshGeometry(const fig::rect& rect);
	private:
		fig::color_ref_with_alpha _colorLeft {};
		fig::color_ref_with_alpha _colorRight {};
		fig::rect _lastRect {};
		fig::texture_ptr _pTexture;

		std::vector<fig::vertex> _vertices {};
	};
}