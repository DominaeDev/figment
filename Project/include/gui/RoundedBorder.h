#ifndef BORDER_H__
#define BORDER_H__
#pragma once

#include "Control.h"

namespace fig::gui
{
	class RoundedBorder : public Control
	{
	public:
		RoundedBorder(Control* pParent, float radius, float thickness, Color color);

		void OnRender(Renderer* pRenderer);
		void SetColor(Color color);

	private:
		void RefreshGeometry(Rectf rect);

		Color _color {};
		Rectf _lastRect {};
		Texture* _pTexture = nullptr;
		float _thickness = 0;
		float _radius = 0;

		std::vector<Vertex> _vertices {};
		std::vector<int> _indices {};
	};
}

#endif