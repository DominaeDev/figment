#pragma once

#include "Types.h"
#include "CustomRenderer.h"

namespace fig::gui
{
	class RoundedBorderRenderer : public CustomRenderer
	{
	public:
		explicit RoundedBorderRenderer(float radius, float thickness, Color color = Colors::White);

		void Render(Renderer* pRenderer, Rectf rect);

	private:
		void RefreshGeometry(Rectf rect);

		Rectf _lastRect {};
		Texture* _pTexture = nullptr;
		float _thickness = 0;
		float _radius = 0;

		std::vector<Vertex> _vertices {};
		std::vector<int> _indices {};
	};
}