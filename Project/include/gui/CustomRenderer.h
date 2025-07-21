#pragma once

#include "Types.h"
#include "Graphics.h"

struct SDL_Renderer;

class CustomRenderer
{
public:
	virtual void Render(Renderer* pRenderer, Rectf rect) = 0;
	virtual ~CustomRenderer() = default;
};