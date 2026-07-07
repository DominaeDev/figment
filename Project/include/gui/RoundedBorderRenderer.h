#pragma once

#include "Figment.h"
#include "CustomRenderer.h"

namespace fig::gui
{
	class RoundedBorderRenderer : public CustomRenderer
	{
	public:
		explicit RoundedBorderRenderer(float radius, float thickness, Color color = Colors::White);

		void Render(Renderer* pRenderer, const Rectf& rect);

	private:
		void RefreshGeometry(Rectf rect);

		Rectf _lastRect {};
		fig::observer_ptr<Texture> _pTexture;
		float _thickness = 0;
		float _radius = 0;

		std::vector<Vertex> _vertices {};
		std::vector<int> _indices {};
	};
}