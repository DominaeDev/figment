#pragma once

#include "CustomRenderer.h"

namespace fig::gui
{
	class RoundedFillRenderer : public CustomRenderer
	{
	public:
		explicit RoundedFillRenderer(float radius, fig::color color = Colors::White);

		void Render(fig::renderer_ptr pRenderer, fig::rectf rect);

	private:
		void RefreshGeometry(fig::rectf rect);

		fig::rectf _lastRect {};
		float _radius = 0;

		std::vector<fig::vertex> _vertices {};
		std::vector<int> _indices {};
	};
}