#pragma once

#include "Control.h"
#include "Graphics.h"

class HorizontalGradient : public Control
{
public:
	HorizontalGradient(Control* pParent, Color colorLeft, Color colorRight);
	void SetColors(Color colorLeft, Color colorRight);

protected:
	void OnUpdate(float fDeltaTime) override {};
	void OnRender(Renderer* pRenderer) override;

	void RefreshGeometry(Rectf rect);
private:
	Colorf _colorLeft {};
	Colorf _colorRight {};
	Rectf _lastRect {};
	Texture* _pTexture {};

	std::vector<Vertex> _vertices {};
};