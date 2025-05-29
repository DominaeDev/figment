#pragma once

#include "Types.h"
#include "CustomRenderer.h"
#include <vector>

struct SDL_Vertex;

class RoundedBackgroundRenderer : public CustomRenderer
{
public:
	RoundedBackgroundRenderer(float radius, SDL_Color color);
	
	void Draw(SDL_Renderer* pRenderer, SDL_FRect rect);
	void SetColor(SDL_Color color);

private:
	void RefreshGeometry(SDL_FRect rect);

	SDL_Color _color {};
	SDL_FRect _lastRect {};
	float _radius = 0;

	std::vector<SDL_Vertex> _vertices {};
	std::vector<int> _indices {};
};