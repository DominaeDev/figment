#pragma once

#include "CustomRenderer.h"

namespace fig::gui
{
	class RoundedBackgroundRenderer : public CustomRenderer
	{
	public:
		RoundedBackgroundRenderer(float radius, Color color);

		void Render(Renderer* pRenderer, Rectf rect);
		void SetColor(Color color);

	private:
		void RefreshGeometry(Rectf rect);

		Color _color {};
		Rectf _lastRect {};
		float _radius = 0;

		std::vector<Vertex> _vertices {};
		std::vector<int> _indices {};
	};
}