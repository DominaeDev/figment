#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class VerticalGradient : public Control
	{
	public:
		VerticalGradient(ControlPtr pParent, fig::color_ref_with_alpha colorTop, fig::color_ref_with_alpha colorBottom);
		void SetColors(fig::color_ref_with_alpha colorTop, fig::color_ref_with_alpha colorBottom);
		void SetTexture(fig::texture_ptr pTexture);

		void OnRender(fig::renderer_ptr pRenderer) override;
	
	protected:
		void OnUpdate(float fElapsed) override {};
		void RefreshGeometry(fig::rectf rect);
	private:
		fig::color_ref_with_alpha _colorTop {};
		fig::color_ref_with_alpha _colorBottom {};
		fig::rectf _lastRect {};
		fig::texture_ptr _pTexture;
		bool _bInvalid { true };

		std::vector<fig::vertex> _vertices {};
	};
}