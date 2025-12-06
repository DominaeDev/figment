#ifndef ROUNDED_BORDER_RENDERER_H__
#define ROUNDED_BORDER_RENDERER_H__

#include "Types.h"
#include "CustomRenderer.h"

class RoundedBorderRenderer : public CustomRenderer
{
public:
	RoundedBorderRenderer(float radius, float thickness, Color color);
	
	void Render(Renderer* pRenderer, Rectf rect);
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
#endif