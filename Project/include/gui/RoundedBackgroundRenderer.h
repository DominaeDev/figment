#ifndef ROUNDED_BACKGROUND_RENDERER_H__
#define ROUNDED_BACKGROUND_RENDERER_H__

#include "CustomRenderer.h"

class RoundedBackgroundRenderer : public CustomRenderer
{
public:
	RoundedBackgroundRenderer(float radius, Color color);
	
	void Render(Renderer* pRenderer, Rectf rect);
	void SetColor(Color color);

private:
	void RefreshGeometry(Rectf rect);

	Color _color {};
	Rectf _lastRect {};
	float _radius = 0;

	std::vector<Vertex> _vertices {};
	std::vector<int> _indices {};
};
#endif