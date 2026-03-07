#include <pch.h>
#include "gui/TextureStore.h"
#include "fs/FileUtility.h"
#include <SDL3_image/SDL_image.h>

namespace fig::gui
{
	std::map<TextureType, fig::sdl::Surface> TextureStore::_surfaces;
	std::map<TextureType, fig::sdl::Texture> TextureStore::_textures;
	std::map<MaskType, Mask> TextureStore::_masks;

	void TextureStore::Init(Renderer* pRenderer)
	{
		LoadTexture(pRenderer, TextureType::BLANK, "./resources/gui/white.png");
		LoadTexture(pRenderer, TextureType::BORDER, "./resources/gui/line.png");
		LoadTexture(pRenderer, TextureType::ICON_ERROR, "./resources/gui/icon_error_2.png");
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
		LoadTexture(pRenderer, TextureType::CARD_BACKGROUND_DEFAULT, "./resources/gui/card_bg_default.png");
		LoadTexture(pRenderer, TextureType::CARD_BORDER_STYLE_01, "./resources/card_borders/border_01.png");
		LoadTexture(pRenderer, TextureType::CARD_BORDER_STYLE_02, "./resources/card_borders/border_02.png");
		LoadTexture(pRenderer, TextureType::CARD_BORDER_STYLE_03, "./resources/card_borders/border_03.png");
		LoadTexture(pRenderer, TextureType::CARD_BORDER_STYLE_04, "./resources/card_borders/border_04.png");
		LoadTexture(pRenderer, TextureType::CARD_BORDER_STYLE_05, "./resources/card_borders/border_05.png");
		LoadTexture(pRenderer, TextureType::CARD_BORDER_STYLE_06, "./resources/card_borders/border_06.png");

		// Masks
		LoadMask(MaskType::CARD_CORNER_MASK, "./resources/masks/card_corners.mask");

		// Masked textures
		LoadTextureAndMaskCorners(pRenderer, TextureType::CARD_BACKGROUND_EMPTY, MaskType::CARD_CORNER_MASK, "./resources/gui/card_bg_empty.png");
	}

	void TextureStore::Release()
	{
		_textures.clear();
		_surfaces.clear();
	}

	bool TextureStore::LoadTexture(Renderer* pRenderer, TextureType textureId, fig::path filename)
	{
		SurfacePtr pSurface;
		auto itFind = _surfaces.find(textureId);
		if (itFind != _surfaces.cend())
			pSurface = itFind->second.get();
		else
		{
			pSurface = IMG_Load(filename.u8string().c_str());
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

	bool TextureStore::LoadTextureAndMaskCorners(Renderer* pRenderer, TextureType textureId, MaskType maskId, fig::path filename)
	{
		SurfacePtr pSurface;
		auto itFind = _surfaces.find(textureId);
		if (itFind != _surfaces.cend())
			pSurface = itFind->second.get();
		else
		{
			pSurface = IMG_Load(filename.u8string().c_str());
			if (not (bool)pSurface)
				return false; // File not found

			auto& surface = _surfaces[textureId] = fig::sdl::Surface();
			surface.reset(pSurface);

			if (gui::util::MaskCorners(surface, MaskType::CARD_CORNER_MASK))
				pSurface = surface.get();
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

	bool TextureStore::LoadMask(MaskType maskId, fig::path filename)
	{
		auto itFind = _masks.find(maskId);
		if (itFind != _masks.cend())
			return true;

		if (auto maybe_mask = fig::io::ReadFile(filename))
		{
			auto& data = maybe_mask.value();
			auto& mask = _masks[maskId] = Mask {};
			mask.pixels.resize(data.size());
			std::memcpy(mask.pixels.data(), data.data(), data.size());
			mask.width = toUZ(std::sqrt(toI(data.size()))); // assumes square mask
			mask.pitch = mask.width * 1;
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

	MaskPtr TextureStore::GetMask(MaskType id)
	{
		auto itFind = _masks.find(id);
		if (itFind != _masks.end())
			return &itFind->second;
		return nullptr;
	}
}