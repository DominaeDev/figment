#include <pch.h>
#include "gui/TextureStore.h"
#include <SDL3_image/SDL_image.h>

using namespace fig::gui;

std::map<TextureType, fig::sdl::Surface> TextureStore::_surfaces;
std::map<TextureType, fig::sdl::Texture> TextureStore::_textures;

void TextureStore::Init(Renderer* pRenderer)
{
	LoadTexture(pRenderer, TextureType::BLANK, "./resources/gui/white.png");
	LoadTexture(pRenderer, TextureType::BORDER, "./resources/gui/line.png");
	LoadTexture(pRenderer, TextureType::TEXTBOX_BG, "./resources/gui/bg_9grid.png");
	LoadTexture(pRenderer, TextureType::TEXTBOX_BORDER, "./resources/gui/bg_9grid_border.png");
	LoadTexture(pRenderer, TextureType::SPEECH_BUBBLE_LEFT_BG, "./resources/gui/speech_bubble_left_bg.png");
	LoadTexture(pRenderer, TextureType::SPEECH_BUBBLE_LEFT_BORDER, "./resources/gui/speech_bubble_left_border.png");
	LoadTexture(pRenderer, TextureType::SPEECH_BUBBLE_CENTER_BG, "./resources/gui/speech_bubble_center_bg.png");
	LoadTexture(pRenderer, TextureType::SPEECH_BUBBLE_CENTER_BORDER, "./resources/gui/speech_bubble_center_border.png");
	LoadTexture(pRenderer, TextureType::SPEECH_BUBBLE_RIGHT_BG, "./resources/gui/speech_bubble_right_bg.png");
	LoadTexture(pRenderer, TextureType::SPEECH_BUBBLE_RIGHT_BORDER, "./resources/gui/speech_bubble_right_border.png");
	
	LoadTexture(pRenderer, TextureType::CARD_BORDER, "./resources/gui/card_border.png");
	LoadTexture(pRenderer, TextureType::CARD_TAG_BG, "./resources/gui/card_tag_bg.png");
	LoadTexture(pRenderer, TextureType::CARD_ICON_CHAT_COUNTER, "./resources/gui/icon_small_chat.png");
	LoadTexture(pRenderer, TextureType::CARD_BOTTOM_FADE, "./resources/gui/card_bottom_fade.png");
	LoadTexture(pRenderer, TextureType::CARD_ICON_FAVORITE_OFF, "./resources/gui/card_icon_favorite_off.png");
	LoadTexture(pRenderer, TextureType::CARD_DEFAULT_BG, "./resources/gui/default_card_bg.png");
}

void TextureStore::Release()
{
	_textures.clear();
	_surfaces.clear();
}

bool TextureStore::LoadTexture(Renderer* pRenderer, TextureType textureId, const char* filename)
{
	SurfacePtr pSurface;
	auto itFind = _surfaces.find(textureId);
	if (itFind != _surfaces.cend())
		pSurface = itFind->second.get();
	else
	{
		pSurface = IMG_Load(filename);
		if (not (bool)pSurface)
			return false; // File not found
		
		auto& surface = _surfaces[textureId] = fig::sdl::Surface();
		surface.reset(pSurface);
	}

	auto pTexture = SDL_CreateTextureFromSurface(pRenderer, pSurface);
	if (pTexture)
	{
		auto& texture = _textures[textureId] = fig::sdl::Texture();
		texture.reset(pTexture);
		return true;
	}
	return false;
}

TexturePtr TextureStore::GetTexture(TextureType id)
{
	auto itFind = _textures.find(id);
	if (itFind != _textures.end())
		return itFind->second.get();
	return nullptr;
}

SurfacePtr TextureStore::GetImage(TextureType id) noexcept
{
	auto itFind = _surfaces.find(id);
	if (itFind != _surfaces.end())
		return itFind->second.get();
	return nullptr;
}
