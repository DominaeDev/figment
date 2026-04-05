#pragma once

#include "CustomRenderer.h"

namespace fig::gui
{
	class RoundedFillRenderer : public CustomRenderer
	{
	public:
		explicit RoundedFillRenderer(float radius, Color color = Colors::White);

		void Render(Renderer* pRenderer, Rectf rect);

	private:
		void RefreshGeometry(Rectf rect);

		Rectf _lastRect {};
		float _radius = 0;

		std::vector<Vertex> _vertices {};
		std::vector<int> _indices {};
	};
}