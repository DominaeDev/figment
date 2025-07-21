#pragma once

#include "Control.h"
#include "Graphics.h"

class VerticalGradient : public Control
{
public:
	VerticalGradient(Control* pParent, Color colorTop, Color colorBottom);
	void SetColors(Color colorTop, Color colorBottom);

protected:
	void OnUpdate(float fDeltaTime) override {};
	void OnRender(Renderer* pRenderer) override;

	void RefreshGeometry(Rectf rect);
private:
	Colorf _colorTop {};
	Colorf _colorBottom {};
	Rectf _lastRect {};
	Texture* _pTexture {};

	std::vector<Vertex> _vertices {};
};