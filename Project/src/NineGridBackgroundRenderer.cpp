#include "NineGridBackgroundRenderer.h"
#include "TextureStore.h"
#include "Utility.h"

NineGridBackgroundRenderer::NineGridBackgroundRenderer(float cornerSize, SDL_Color bgColor, SDL_Color borderColor) :
	_cornerSize(cornerSize),
	_bgColor(bgColor),
	_borderColor(borderColor)
{
	_pBGTexture = TextureStore::GetTexture(Texture::BG_9GRID);
	_pBorderTexture = TextureStore::GetTexture(Texture::BORDER_9GRID);
}

void NineGridBackgroundRenderer::Draw(SDL_Renderer* pRenderer, SDL_FRect rect)
{
	auto expandedRect = Rect_Expand(rect, 5.0f);
	
	SDL_SetTextureColorMod(_pBGTexture, _bgColor.r, _bgColor.g, _bgColor.b);
	SDL_RenderTexture9Grid(pRenderer, _pBGTexture, nullptr, 64.0f, 64.0f, 64.0f, 64.0f, _cornerSize / 20.0f, &expandedRect);

	SDL_SetTextureColorMod(_pBorderTexture, _borderColor.r, _borderColor.g, _borderColor.b);
	SDL_RenderTexture9Grid(pRenderer, _pBorderTexture, nullptr, 64.0f, 64.0f, 64.0f, 64.0f, _cornerSize / 20.0f, &expandedRect);
}