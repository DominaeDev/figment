#include <pch.h>
#include "gui/AppResources.h"
#include "fs/FileUtility.h"
#include "fs/BinaryReader.h"
#include "model/UserProfile.h"
#include <SDL3_image/SDL_image.h>
#include <cassert>

using namespace fig::io;

namespace fig::gui
{
	std::map<TextureType, fig::sdl::Surface> AppResources::_surfaces;
	std::map<TextureType, fig::sdl::Texture> AppResources::_textures;
	std::map<MaskType, Mask> AppResources::_masks;
	std::map<fig::uuid, fig::sdl::Surface> AppResources::_profileSurfaces;
	std::map<fig::uuid, fig::sdl::Texture> AppResources::_profileTextures;

	void AppResources::Init(Renderer* pRenderer)
	{
		// Masks
		LoadMask(MaskType::CARD_CORNER_MASK, "./resources/gui/masks/mask_card_corners.mask");

		// Generic
		LoadTexture(pRenderer, TextureType::BLANK, "./resources/gui/white.png");
		LoadTexture(pRenderer, TextureType::BORDER, "./resources/gui/line.png");
		LoadTexture(pRenderer, TextureType::LOGO_SMALL, "./resources/gui/logo_small.png");
		LoadTexture(pRenderer, TextureType::ROUNDED_BACKGROUND, "./resources/gui/bg_rounded.png");
		LoadTexture(pRenderer, TextureType::ROUNDED_BORDER, "./resources/gui/border_rounded.png");

		// Icons
		LoadTexture(pRenderer, TextureType::ICON_ERROR, "./resources/gui/icons/icon_error_2.png");
		LoadTexture(pRenderer, TextureType::ICON_SIDEBAR, "./resources/gui/icons/icon_side_panel.png");
		LoadTexture(pRenderer, TextureType::ICON_SIDEBAR_COLLAPSE, "./resources/gui/icons/icon_side_panel_collapse.png");
		LoadTexture(pRenderer, TextureType::ICON_SIDEBAR_EXPAND, "./resources/gui/icons/icon_side_panel_expand.png");
		LoadTexture(pRenderer, TextureType::ICON_SETTINGS, "./resources/gui/icons/icon_settings.png");
		LoadTexture(pRenderer, TextureType::ICON_SEARCH, "./resources/gui/icons/icon_search.png");
		LoadTexture(pRenderer, TextureType::ICON_MENU, "./resources/gui/icons/icon_menu.png");
		LoadTexture(pRenderer, TextureType::ICON_MENU_CHATS, "./resources/gui/icons/icon_chats.png");
		LoadTexture(pRenderer, TextureType::ICON_MENU_CHARACTERS, "./resources/gui/icons/icon_characters.png");
		LoadTexture(pRenderer, TextureType::ICON_MENU_SCENARIOS, "./resources/gui/icons/icon_scenarios.png");
		LoadTexture(pRenderer, TextureType::ICON_GRID_SMALL, "./resources/gui/icons/icon_grid_small.png");
		LoadTexture(pRenderer, TextureType::ICON_GRID_LARGE, "./resources/gui/icons/icon_grid_large.png");

		// Chat
		LoadTexture(pRenderer, TextureType::TEXTBOX_BG, "./resources/gui/chat/bg_9grid.png");
		LoadTexture(pRenderer, TextureType::TEXTBOX_BORDER, "./resources/gui/chat/bg_9grid_border.png");
		LoadTexture(pRenderer, TextureType::SPEECH_BUBBLE_LEFT_BG, "./resources/gui/chat/speech_bubble_left_bg.png");
		LoadTexture(pRenderer, TextureType::SPEECH_BUBBLE_LEFT_BORDER, "./resources/gui/chat/speech_bubble_left_border.png");
		LoadTexture(pRenderer, TextureType::SPEECH_BUBBLE_CENTER_BG, "./resources/gui/chat/speech_bubble_center_bg.png");
		LoadTexture(pRenderer, TextureType::SPEECH_BUBBLE_CENTER_BORDER, "./resources/gui/chat/speech_bubble_center_border.png");
		LoadTexture(pRenderer, TextureType::SPEECH_BUBBLE_RIGHT_BG, "./resources/gui/chat/speech_bubble_right_bg.png");
		LoadTexture(pRenderer, TextureType::SPEECH_BUBBLE_RIGHT_BORDER, "./resources/gui/chat/speech_bubble_right_border.png");

		// Cards
		LoadTexture(pRenderer, TextureType::CARD_BORDER, "./resources/gui/card/card_border.png");
		LoadTexture(pRenderer, TextureType::CARD_TAG_BG, "./resources/gui/card/card_tag_bg.png");
		LoadTexture(pRenderer, TextureType::CARD_ICON_CHAT_COUNTER, "./resources/gui/card/icon_small_chat.png");
		LoadTexture(pRenderer, TextureType::CARD_BOTTOM_FADE, "./resources/gui/card/card_bottom_fade.png");
		LoadTexture(pRenderer, TextureType::CARD_BOTTOM_FADE_SMALL, "./resources/gui/card/card_bottom_fade_small.png");
		LoadTexture(pRenderer, TextureType::CARD_ICON_FAVORITE_OFF, "./resources/gui/card/card_icon_favorite_off.png");
		LoadTexture(pRenderer, TextureType::CARD_BACKGROUND_DEFAULT, "./resources/gui/card/card_bg_default.png");
		LoadTexture(pRenderer, TextureType::CARD_BORDER_STYLE_01, "./resources/gui/card/borders/border_01.png");
		LoadTexture(pRenderer, TextureType::CARD_BORDER_STYLE_02, "./resources/gui/card/borders/border_02.png");
		LoadTexture(pRenderer, TextureType::CARD_BORDER_STYLE_03, "./resources/gui/card/borders/border_03.png");
		LoadTexture(pRenderer, TextureType::CARD_BORDER_STYLE_04, "./resources/gui/card/borders/border_04.png");
		LoadTexture(pRenderer, TextureType::CARD_BORDER_STYLE_05, "./resources/gui/card/borders/border_05.png");
		LoadTexture(pRenderer, TextureType::CARD_BORDER_STYLE_06, "./resources/gui/card/borders/border_06.png");
		LoadTextureAndMaskCorners(pRenderer, TextureType::CARD_BACKGROUND_EMPTY, MaskType::CARD_CORNER_MASK, "./resources/gui/card/card_bg_empty.png");

		LoadTexture(pRenderer, TextureType::PROFILE_DEFAULT_IMAGE, "./resources/gui/images/default_portrait.png");
		LoadTexture(pRenderer, TextureType::PROFILE_MASK, "./resources/gui/masks/mask_circle256.png");
	}

	void AppResources::Release()
	{
		_textures.clear();
		_surfaces.clear();
	}

	bool AppResources::LoadTexture(Renderer* pRenderer, TextureType textureId, fig::path filename)
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

		assert(false);
		return false;
	}

	bool AppResources::LoadTextureAndMaskCorners(Renderer* pRenderer, TextureType textureId, MaskType maskId, fig::path filename)
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

	TexturePtr AppResources::GetTexture(TextureType id)
	{
		auto itFind = _textures.find(id);
		if (itFind != _textures.end())
			return itFind->second.get();
		return nullptr;
	}

	SurfacePtr AppResources::GetImage(TextureType id) noexcept
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

	TexturePtr AppResources::GetUserProfileImage(Renderer* pRenderer, const fig::user::UserProfile& profile)
	{
		auto itFind = _profileTextures.find(profile.id);
		if (itFind != _profileTextures.end())
			return itFind->second.get();

		SurfacePtr pSurface = nullptr;
		if (auto itSurf = _profileSurfaces.find(profile.id); itSurf != _profileSurfaces.end())
		{
			pSurface = itSurf->second.get();
		}
		else
		{
			// Load from disk
			if (auto result = BinaryReader::ReadProfileFile(profile, std::format("{}.{}", Constants::Paths::ProfileImageFileName, Constants::Paths::ProfileImageFileExt)); result.has_value())
			{
				const auto& asset = result.value();
				auto width = asset.get_meta<uint16_t>(fig::io::data::MetaTag::ImageWidth).value_or(0);
				auto height = asset.get_meta<uint16_t>(fig::io::data::MetaTag::ImageHeight).value_or(0);
				auto format = static_cast<fig::gui::ImageFormat>(asset.get_meta<uint8_t>(fig::io::data::MetaTag::ImageFormat).value_or(0));

				if (auto image = fig::gui::util::SurfaceFromBytes(width, height, format, asset.data); not image.empty())
				{
					pSurface = image.get();
					_profileSurfaces[profile.id] = std::move(image);
				}
			}
		}

		if (pSurface)
		{
			if (auto pTexture = SDL_CreateTextureFromSurface(pRenderer, pSurface))
			{
				auto& texture = _profileTextures[profile.id] = fig::sdl::Texture::create_and_claim(pTexture);
				return texture.get();
			}
		}

		return nullptr;
	}
}