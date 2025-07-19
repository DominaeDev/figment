#pragma once

#include "Types.h"
#include "CustomRenderer.h"

struct SDL_Texture;

class NineGridBackgroundRenderer : public CustomRenderer
{
public:
	NineGridBackgroundRenderer(int cornerPixels = 64);
	NineGridBackgroundRenderer(std::array<float, 4> corners);
	
	void Draw(SDL_Renderer* pRenderer, SDL_FRect rect);
	void SetCornerSize(float cornerSize);
	void SetColors(SDL_Color bgColor, SDL_Color borderColor);
	void SetTextures(SDL_Texture* bgTexture, SDL_Texture* borderTexture);

private:
	std::array<float, 4> _cornerPixels = { 64, 64, 64, 64 };
	float _cornerSize = 20.0f;
	SDL_Color _bgColor {};
	SDL_Color _borderColor {};
	SDL_Texture* _pBGTexture = nullptr;
	SDL_Texture* _pBorderTexture = nullptr;
};