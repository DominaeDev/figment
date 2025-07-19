#include "gui/TextureStore.h"
#include <SDL3_image/SDL_image.h>

std::map<Texture, SDL_Texture*> TextureStore::_textures;

void TextureStore::Init(SDL_Renderer* pRenderer)
{
	LoadTexture(pRenderer, Texture::BORDER, "./resources/gui/line.png");
	LoadTexture(pRenderer, Texture::BG_9GRID, "./resources/gui/bg_9grid.png");
	LoadTexture(pRenderer, Texture::BORDER_9GRID, "./resources/gui/bg_9grid_border.png");
}

void TextureStore::Release()
{
	for (auto it : _textures)
	{
		SDL_DestroyTexture(it.second);
	}
}

bool TextureStore::LoadTexture(SDL_Renderer* pRenderer, Texture textureId, const char* filename)
{
	auto pSurface = IMG_Load(filename);
	if (!pSurface)
		return false;

	auto pTexture = SDL_CreateTextureFromSurface(pRenderer, pSurface);
	SDL_DestroySurface(pSurface);

	if (pTexture)
	{
		_textures[textureId] = pTexture;
		return true;
	}
	return false;
}

SDL_Texture* TextureStore::GetTexture(Texture id)
{
	auto itFind = _textures.find(id);
	if (itFind != std::end(_textures))
		return itFind->second;
	return nullptr;
}