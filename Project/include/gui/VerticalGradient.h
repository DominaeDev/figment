#pragma once

#include "Control.h"

namespace fig::gui
{
	class VerticalGradient : public Control
	{
	public:
		VerticalGradient(ParentPtr pParent, Color colorTop, Color colorBottom);
		void SetColors(Color colorTop, Color colorBottom);
		void SetTexture(fig::observer_ptr<Texture> pTexture);

		void OnRender(Renderer* pRenderer) override;
	protected:
		void OnUpdate(float fElapsed) override {};

		void RefreshGeometry(Rectf rect);
	private:
		Colorf _colorTop {};
		Colorf _colorBottom {};
		Rectf _lastRect {};
		fig::observer_ptr<Texture> _pTexture;

		std::vector<Vertex> _vertices {};
	};
}