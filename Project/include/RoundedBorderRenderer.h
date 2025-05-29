#pragma once

#include "Types.h"
#include "CustomRenderer.h"
#include <vector>

struct SDL_Vertex;
struct SDL_Texture;

class RoundedBorderRenderer : public CustomRenderer
{
public:
	RoundedBorderRenderer(float radius, float thickness, SDL_Color color);
	
	void Draw(SDL_Renderer* pRenderer, SDL_FRect rect);
	void SetColor(SDL_Color color);

private:
	void RefreshGeometry(SDL_FRect rect);

	SDL_Color _color {};
	SDL_FRect _lastRect {};
	SDL_Texture* _pTexture = nullptr;
	float _thickness = 0;
	float _radius = 0;

	std::vector<SDL_Vertex> _vertices {};
	std::vector<int> _indices {};
};