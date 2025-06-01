#pragma once

#include "Types.h"
#include "CustomRenderer.h"

struct SDL_Texture;

class NineGridBackgroundRenderer : public CustomRenderer
{
public:
	NineGridBackgroundRenderer(float cornerSize, SDL_Color bgColor, SDL_Color borderColor);
	
	void Draw(SDL_Renderer* pRenderer, SDL_FRect rect);
	void SetColors(SDL_Color bgColor, SDL_Color borderColor);

private:
	float _cornerSize;
	SDL_Color _bgColor {};
	SDL_Color _borderColor {};
	SDL_Texture* _pBGTexture;
	SDL_Texture* _pBorderTexture;
};