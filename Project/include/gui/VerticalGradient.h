#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class VerticalGradient : public Control
	{
	public:
		VerticalGradient(ControlPtr pParent, fig::color_ref colorTop, fig::color_ref colorBottom);
		void SetColors(fig::color_ref colorTop, fig::color_ref colorBottom);
		void SetTexture(fig::texture_ptr pTexture);

		void OnRender(fig::renderer_ptr pRenderer) override;
	
	protected:
		void OnUpdate(float fElapsed) override {};
		void RefreshGeometry(fig::rectf rect);
	private:
		fig::color_ref _colorTop {};
		fig::color_ref _colorBottom {};
		fig::rectf _lastRect {};
		fig::texture_ptr _pTexture;
		bool _bInvalid { true };

		std::vector<fig::vertex> _vertices {};
	};
}