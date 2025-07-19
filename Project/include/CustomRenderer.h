#pragma once

#include "Types.h"
#include "Graphics.h"

struct SDL_Renderer;
struct SDL_FRect;

class CustomRenderer
{
public:
	virtual void Draw(SDL_Renderer* pRenderer, SDL_FRect rect) = 0;
	virtual ~CustomRenderer() = default;
};