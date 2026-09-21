#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class RoundedBorder : public Control
	{
	public:
		RoundedBorder(ControlPtr pParent, float radius, float thickness, fig::color_ref color);

		void OnRender(fig::renderer_ptr pRenderer);
		void SetColor(fig::color_ref color);

	private:
		void RefreshGeometry(fig::rectf rect);

		fig::color_ref _color {};
		fig::rectf _lastRect {};
		fig::texture_ptr _pTexture;
		float _thickness = 0;
		float _radius = 0;

		std::vector<fig::vertex> _vertices {};
		std::vector<int> _indices {};
	};
}
