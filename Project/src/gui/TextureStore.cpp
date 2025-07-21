#include "gui/TextureStore.h"
#include <SDL3_image/SDL_image.h>

std::map<Texture, SDL_Texture*> TextureStore::_textures;

void TextureStore::Init(SDL_Renderer* pRenderer)
{
	LoadTexture(pRenderer, Texture::BLANK, "./resources/gui/white.png");
	LoadTexture(pRenderer, Texture::BORDER, "./resources/gui/line.png");
	LoadTexture(pRenderer, Texture::TEXTBOX_BG, "./resources/gui/bg_9grid.png");
	LoadTexture(pRenderer, Texture::TEXTBOX_BORDER, "./resources/gui/bg_9grid_border.png");
	LoadTexture(pRenderer, Texture::SPEECH_BUBBLE_LEFT_BG, "./resources/gui/speech_bubble_left_bg.png");
	LoadTexture(pRenderer, Texture::SPEECH_BUBBLE_LEFT_BORDER, "./resources/gui/speech_bubble_left_border.png");
	LoadTexture(pRenderer, Texture::SPEECH_BUBBLE_CENTER_BG, "./resources/gui/speech_bubble_center_bg.png");
	LoadTexture(pRenderer, Texture::SPEECH_BUBBLE_CENTER_BORDER, "./resources/gui/speech_bubble_center_border.png");
	LoadTexture(pRenderer, Texture::SPEECH_BUBBLE_RIGHT_BG, "./resources/gui/speech_bubble_right_bg.png");
	LoadTexture(pRenderer, Texture::SPEECH_BUBBLE_RIGHT_BORDER, "./resources/gui/speech_bubble_right_border.png");
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