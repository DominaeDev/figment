#include <pch.h>
#include "gui/AppResources.h"
#include "io/FileUtility.h"
#include "io/AssetFileReader.h"
#include "user/UserProfile.h"
#include <SDL3_image/SDL_image.h>
#include <cassert>

using namespace fig::io;

namespace fig
{
	std::map<Resource, fig::sdl::Surface> AppResources::_surfaces;
	std::map<Resource, fig::sdl::Texture> AppResources::_textures;
	std::map<MaskType, Mask> AppResources::_masks;

	void AppResources::Init(fig::renderer_ptr pRenderer)
	{
		// Masks
		LoadMask(MaskType::CARD_CORNER_MASK, "./resources/gui/masks/mask_card_corners.mask");

		// Generic
		LoadTexture(pRenderer, Resource::BLANK, "./resources/gui/white.png");
		LoadTexture(pRenderer, Resource::BORDER, "./resources/gui/line.png");
		LoadTexture(pRenderer, Resource::LOGO_SMALL, "./resources/gui/logo_small.png");
		LoadTexture(pRenderer, Resource::ROUNDED_BACKGROUND_6PX, "./resources/gui/rounded_bg_6px.png");
		LoadTexture(pRenderer, Resource::ROUNDED_BORDER_6PX, "./resources/gui/rounded_border_6px.png");
		LoadTexture(pRenderer, Resource::THICK_ROUNDED_BORDER_6PX, "./resources/gui/thick_rounded_border_6px.png");
		LoadTexture(pRenderer, Resource::ROUNDED_BACKGROUND_10PX, "./resources/gui/rounded_bg_10px.png");
		LoadTexture(pRenderer, Resource::ROUNDED_BORDER_10PX, "./resources/gui/rounded_border_10px.png");
		LoadTexture(pRenderer, Resource::THICK_ROUNDED_BORDER_10PX, "./resources/gui/thick_rounded_border_10px.png");
		LoadTexture(pRenderer, Resource::BACKGROUND_CIRCLE_48PX , "./resources/gui/circle_48.png");

		// Chat
		LoadTexture(pRenderer, Resource::TEXTBOX_BG, "./resources/gui/chat/bg_9grid.png");
		LoadTexture(pRenderer, Resource::TEXTBOX_BORDER, "./resources/gui/chat/bg_9grid_border.png");
		LoadTexture(pRenderer, Resource::SPEECH_BUBBLE_LEFT_BG, "./resources/gui/chat/speech_bubble_left_bg.png");
		LoadTexture(pRenderer, Resource::SPEECH_BUBBLE_LEFT_BORDER, "./resources/gui/chat/speech_bubble_left_border.png");
		LoadTexture(pRenderer, Resource::SPEECH_BUBBLE_CENTER_BG, "./resources/gui/chat/speech_bubble_center_bg.png");
		LoadTexture(pRenderer, Resource::SPEECH_BUBBLE_CENTER_BORDER, "./resources/gui/chat/speech_bubble_center_border.png");
		LoadTexture(pRenderer, Resource::SPEECH_BUBBLE_RIGHT_BG, "./resources/gui/chat/speech_bubble_right_bg.png");
		LoadTexture(pRenderer, Resource::SPEECH_BUBBLE_RIGHT_BORDER, "./resources/gui/chat/speech_bubble_right_border.png");

		// Cards
		LoadTexture(pRenderer, Resource::CARD_FILL, "./resources/gui/card/card_fill.png");
		LoadTexture(pRenderer, Resource::CARD_BORDER, "./resources/gui/card/card_border.png");
		LoadTexture(pRenderer, Resource::CARD_TAG_BG, "./resources/gui/card/card_tag_bg.png");
		LoadTexture(pRenderer, Resource::CARD_ICON_CHAT_COUNTER, "./resources/gui/card/icon_small_chat.png");
		LoadTexture(pRenderer, Resource::CARD_BOTTOM_FADE, "./resources/gui/card/card_bottom_fade.png");
		LoadTexture(pRenderer, Resource::CARD_BOTTOM_FADE_SMALL, "./resources/gui/card/card_bottom_fade_small.png");
		LoadTexture(pRenderer, Resource::CARD_ICON_STAR, "./resources/gui/card/icon_star_large.png");
		LoadTexture(pRenderer, Resource::CARD_ICON_STAR_SMALL, "./resources/gui/card/icon_star_small.png");
		LoadTexture(pRenderer, Resource::CARD_BACKGROUND_DEFAULT, "./resources/gui/card/card_bg_default.png");
		LoadTexture(pRenderer, Resource::CARD_BORDER_STYLE_01, "./resources/gui/card/borders/border_01.png");
		LoadTexture(pRenderer, Resource::CARD_BORDER_STYLE_02, "./resources/gui/card/borders/border_02.png");
		LoadTexture(pRenderer, Resource::CARD_BORDER_STYLE_03, "./resources/gui/card/borders/border_03.png");
		LoadTexture(pRenderer, Resource::CARD_BORDER_STYLE_04, "./resources/gui/card/borders/border_04.png");
		LoadTexture(pRenderer, Resource::CARD_BORDER_STYLE_05, "./resources/gui/card/borders/border_05.png");
		LoadTexture(pRenderer, Resource::CARD_BORDER_STYLE_06, "./resources/gui/card/borders/border_06.png");
		LoadTexture(pRenderer, Resource::CARD_BACKGROUND_EMPTY, "./resources/gui/card/card_bg_empty.png");
		LoadTexture(pRenderer, Resource::CARD_COVER_CHARACTER, "./resources/gui/card/card_default_cover_character.png");
		LoadTexture(pRenderer, Resource::CARD_COVER_STORY, "./resources/gui/card/card_default_cover_story.png");
		LoadTexture(pRenderer, Resource::CARD_COVER_WORLD, "./resources/gui/card/card_default_cover_world.png");

		LoadTexture(pRenderer, Resource::SQUARE_BACKGROUND_DEFAULT, "./resources/gui/chat/square_bg_default.png");
		LoadTexture(pRenderer, Resource::PROFILE_DEFAULT_IMAGE, "./resources/gui/chat/default_portrait.png");
		
		LoadTexture(pRenderer, Resource::MASK_CARD, "./resources/gui/masks/mask_card_corners.png");
		LoadTexture(pRenderer, Resource::MASK_CIRCLE, "./resources/gui/masks/mask_circle256.png");
		LoadTexture(pRenderer, Resource::MASK_SMALL_PORTRAIT_48PX, "./resources/gui/masks/mask_small_portrait_48px.png");
		LoadTexture(pRenderer, Resource::MASK_SMALL_PORTRAIT_56PX, "./resources/gui/masks/mask_small_portrait_56px.png");
		LoadTexture(pRenderer, Resource::MASK_THUMBNAIL_PORTRAIT, "./resources/gui/masks/mask_thumbnail_portrait.png");
		LoadTexture(pRenderer, Resource::MASK_THUMBNAIL_LANDSCAPE, "./resources/gui/masks/mask_thumbnail_landscape.png");
		
		LoadTexture(pRenderer, Resource::MASK_GRADIENT_EASE_IN_CUBIC_LEFT, "./resources/gui/masks/mask_gradient_ease_in_cubic_l.png");
		LoadTexture(pRenderer, Resource::MASK_GRADIENT_EASE_IN_CUBIC_RIGHT, "./resources/gui/masks/mask_gradient_ease_in_cubic_r.png");
		LoadTexture(pRenderer, Resource::MASK_GRADIENT_EASE_IN_OUT_SINE_LEFT, "./resources/gui/masks/mask_gradient_ease_in_out_sine_l.png");
		LoadTexture(pRenderer, Resource::MASK_GRADIENT_EASE_IN_OUT_SINE_RIGHT, "./resources/gui/masks/mask_gradient_ease_in_out_sine_r.png");

		LoadTexture(pRenderer, Resource::SLIDER_BAR_BG, "./resources/gui/slider_bar_bg.png");
		LoadTexture(pRenderer, Resource::SLIDER_BAR_BORDER, "./resources/gui/slider_bar_border.png");
		LoadTexture(pRenderer, Resource::SLIDER_THUMB_BG, "./resources/gui/slider_thumb_bg.png");
		LoadTexture(pRenderer, Resource::SLIDER_THUMB_BORDER, "./resources/gui/slider_thumb_border.png");
		
		LoadTexture(pRenderer, Resource::HORIZONTAL_BAR, "./resources/gui/horizontal_bar.png");
		LoadTexture(pRenderer, Resource::HORIZONTAL_BAR_SMALL, "./resources/gui/horizontal_bar_small.png");
		LoadTexture(pRenderer, Resource::VERTICAL_BAR_SMALL, "./resources/gui/vertical_bar_small.png");
	}

	void AppResources::Release()
	{
		_textures.clear();
		_surfaces.clear();
	}

	bool AppResources::LoadTexture(fig::renderer_ptr pRenderer, Resource textureId, fig::path filename)
	{
		fig::surface_ptr pSurface;
		auto itFind = _surfaces.find(textureId);
		if (itFind != _surfaces.cend())
			pSurface = itFind->second.get();
		else
		{
			pSurface = IMG_Load(filename.u8string().c_str());
			if (not (bool)pSurface)
				return false; // File not found

			_surfaces[textureId] = fig::sdl::Surface::from_ptr(pSurface);
		}

		auto pTexture = CreateTexture(pRenderer, pSurface);
		if (pTexture)
		{
			_textures[textureId] = std::move(pTexture);
			return true;
		}

		assert(false);
		return false;
	}

	bool AppResources::LoadTextureAndMaskCorners(fig::renderer_ptr pRenderer, Resource textureId, MaskType maskId, fig::path filename)
	{
		fig::surface_ptr pSurface;
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

			if constexpr (Disabled)
			{
				if (MaskCorners(surface, MaskType::CARD_CORNER_MASK))
					pSurface = surface.get();
			}
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

	bool AppResources::LoadMask(MaskType maskId, fig::path filename)
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

	fig::texture_ptr AppResources::GetTexture(Resource id)
	{
		auto itFind = _textures.find(id);
		if (itFind != _textures.end())
			return itFind->second.get();
		return nullptr;
	}

	fig::surface_ptr AppResources::GetImage(Resource id) noexcept
	{
		auto itFind = _surfaces.find(id);
		if (itFind != _surfaces.end())
			return itFind->second.get();
		return nullptr;
	}

	MaskPtr AppResources::GetMask(MaskType id)
	{
		auto itFind = _masks.find(id);
		if (itFind != _masks.end())
			return &itFind->second;
		return nullptr;
	}

}