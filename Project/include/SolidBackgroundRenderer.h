#pragma once

#include "Types.h"
#include "BackgroundRenderer.h"

class SolidBackgroundRenderer : public BackgroundRenderer
{
public:
	SolidBackgroundRenderer(SDL_Color color);
	
	void DrawBackground(SDL_Renderer* pRenderer, SDL_FRect rect);
	void SetColor(SDL_Color color);

private:
	SDL_Color _color {};
};