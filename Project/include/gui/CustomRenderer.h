#ifndef CUSTOM_RENDERER_H__
#define CUSTOM_RENDERER_H__

#include "Types.h"
#include "Graphics.h"

struct SDL_Renderer;

class CustomRenderer
{
public:
	virtual void Render(Renderer* pRenderer, Rectf rect) = 0;
	virtual ~CustomRenderer() = default;
};

#endif