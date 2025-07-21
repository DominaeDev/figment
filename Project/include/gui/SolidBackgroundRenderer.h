#pragma once

#include "Types.h"
#include "CustomRenderer.h"

class SolidBackgroundRenderer : public CustomRenderer
{
public:
	SolidBackgroundRenderer(Color color);
	
	void Render(Renderer* pRenderer, Rectf rect);
	void SetColor(Color color);

private:
	Color _color {};
};