#pragma once

#include "Types.h"
#include "CustomRenderer.h"

class SolidBackgroundRenderer : public CustomRenderer
{
public:
	SolidBackgroundRenderer(SDL_Color color);
	
	void Draw(SDL_Renderer* pRenderer, SDL_FRect rect);
	void SetColor(SDL_Color color);

private:
	SDL_Color _color {};
};