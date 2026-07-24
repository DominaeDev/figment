#pragma once

#include "Control.h"

namespace fig::gui
{
	class RoundedBorder : public Control
	{
	public:
		RoundedBorder(ControlPtr pParent, float radius, float thickness, fig::color color);

		void OnRender(fig::renderer_ptr pRenderer);
		void SetColor(fig::color color);

	private:
		void RefreshGeometry(fig::rectf rect);

		fig::color _color {};
		fig::rectf _lastRect {};
		fig::texture_ptr _pTexture;
		float _thickness = 0;
		float _radius = 0;

		std::vector<fig::vertex> _vertices {};
		std::vector<int> _indices {};
	};
}
