#pragma once

struct SDL_Renderer;
struct SDL_FRect;

class BackgroundRenderer
{
public:
	virtual void DrawBackground(SDL_Renderer* pRenderer, SDL_FRect rect) = 0;
	virtual ~BackgroundRenderer() = default;
};