#pragma once

#include "Control.h"

namespace fig::gui
{
	class RoundedBorder : public Control
	{
	public:
		RoundedBorder(LayoutElement* pParent, float radius, float thickness, Color color);

		void OnRender(Renderer* pRenderer);
		void SetColor(Color color);

	private:
		void RefreshGeometry(Rectf rect);

		Color _color {};
		Rectf _lastRect {};
		fig::observer_ptr<Texture> _pTexture;
		float _thickness = 0;
		float _radius = 0;

		std::vector<Vertex> _vertices {};
		std::vector<int> _indices {};
	};
}
