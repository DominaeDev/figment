#ifndef SOLID_BACKGROUND_RENDERER_H__
#define SOLID_BACKGROUND_RENDERER_H__

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

#endif