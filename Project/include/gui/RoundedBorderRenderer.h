#pragma once

#include "Figment.h"
#include "CustomRenderer.h"

namespace fig::gui
{
	class RoundedBorderRenderer : public CustomRenderer
	{
	public:
		explicit RoundedBorderRenderer(float radius, float thickness, fig::color color = Color::White);

		void Render(fig::renderer_ptr pRenderer, const fig::rectf& rect);

	private:
		void RefreshGeometry(fig::rectf rect);

		fig::rectf _lastRect {};
		fig::texture_ptr _pTexture;
		float _thickness = 0;
		float _radius = 0;

		std::vector<fig::vertex> _vertices {};
		std::vector<int> _indices {};
	};
}