#pragma once

#include "Control.h"

namespace fig::gui
{
	class HorizontalGradient : public Control
	{
	public:
		HorizontalGradient(LayoutElement* pParent, Color colorLeft, Color colorRight);
		void SetColors(Color colorLeft, Color colorRight);

	protected:
		void OnUpdate(float fElapsed) override {};
		void OnRender(Renderer* pRenderer) override;

		void RefreshGeometry(const Rect& rect);
	private:
		Colorf _colorLeft {};
		Colorf _colorRight {};
		Rect _lastRect {};
		Texture* _pTexture {};

		std::vector<Vertex> _vertices {};
	};
}