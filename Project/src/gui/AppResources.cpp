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
	std::map<fig::uuid, fig::sdl::Surface> AppResources::_profileSurfaces;
	std::map<fig::uuid, fig::sdl::Texture> AppResources::_profileTextures;

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
		LoadTexture(pRenderer, Resource::ROUNDED_BACKGROUND_10PX, "./resources/gui/rounded_bg_10px.png");
		LoadTexture(pRenderer, Resource::ROUNDED_BORDER_10PX, "./resources/gui/rounded_border_10px.png");
		LoadTexture(pRenderer, Resource::SUBMENU_ARROW , "./resources/gui/submenu_arrow.png");
		LoadTexture(pRenderer, Resource::BACKGROUND_CIRCLE_48PX , "./resources/gui/circle_48.png");

		// Icons
		LoadTexture(pRenderer, Resource::ICON_ERROR, "./resources/gui/icons/icon_error_2.png");
		LoadTexture(pRenderer, Resource::ICON_SIDEBAR, "./resources/gui/icons/icon_side_panel.png");
		LoadTexture(pRenderer, Resource::ICON_SIDEBAR_COLLAPSE, "./resources/gui/icons/icon_side_panel_collapse.png");
		LoadTexture(pRenderer, Resource::ICON_SIDEBAR_EXPAND, "./resources/gui/icons/icon_side_panel_expand.png");
		LoadTexture(pRenderer, Resource::ICON_SETTINGS, "./resources/gui/icons/icon_settings.png");
		LoadTexture(pRenderer, Resource::ICON_SEARCH, "./resources/gui/icons/icon_search.png");
		LoadTexture(pRenderer, Resource::ICON_MENU, "./resources/gui/icons/icon_menu.png");
		LoadTexture(pRenderer, Resource::ICON_MENU_CHATS, "./resources/gui/icons/icon_chats.png");
		LoadTexture(pRenderer, Resource::ICON_MENU_CHARACTERS, "./resources/gui/icons/icon_characters.png");
		LoadTexture(pRenderer, Resource::ICON_MENU_SCENARIOS, "./resources/gui/icons/icon_scenarios.png");
		LoadTexture(pRenderer, Resource::ICON_MENU_WORLDS, "./resources/gui/icons/icon_worlds.png");
		LoadTexture(pRenderer, Resource::ICON_MENU_MODELS, "./resources/gui/icons/icon_models.png");
		LoadTexture(pRenderer, Resource::ICON_MENU_CHATS_SMALL, "./resources/gui/icons/icon_chats_small.png");
		LoadTexture(pRenderer, Resource::ICON_MENU_CHARACTERS_SMALL, "./resources/gui/icons/icon_characters_small.png");
		LoadTexture(pRenderer, Resource::ICON_MENU_SCENARIOS_SMALL, "./resources/gui/icons/icon_scenarios_small.png");
		LoadTexture(pRenderer, Resource::ICON_MENU_WORLDS_SMALL, "./resources/gui/icons/icon_worlds_small.png");
		LoadTexture(pRenderer, Resource::ICON_MENU_MODELS_SMALL, "./resources/gui/icons/icon_models_small.png");
		LoadTexture(pRenderer, Resource::ICON_GRID_SMALL, "./resources/gui/icons/icon_grid_small.png");
		LoadTexture(pRenderer, Resource::ICON_GRID_LARGE, "./resources/gui/icons/icon_grid_large.png");
		LoadTexture(pRenderer, Resource::ICON_TAG, "./resources/gui/icons/icon_tag.png");
		LoadTexture(pRenderer, Resource::ICON_LOGOUT, "./resources/gui/icons/icon_logout.png");
		LoadTexture(pRenderer, Resource::ICON_LOCK, "./resources/gui/icons/icon_lock.png");
		LoadTexture(pRenderer, Resource::ICON_ARROW_LEFT, "./resources/gui/icons/icon_arrow_left.png");
		LoadTexture(pRenderer, Resource::ICON_ARROW_RIGHT, "./resources/gui/icons/icon_arrow_right.png");
		LoadTexture(pRenderer, Resource::ICON_CHEVRON_LEFT, "./resources/gui/icons/icon_chevron_left.png");
		LoadTexture(pRenderer, Resource::ICON_CHEVRON_RIGHT, "./resources/gui/icons/icon_chevron_right.png");
		LoadTexture(pRenderer, Resource::ICON_HOME, "./resources/gui/icons/icon_home.png");
		LoadTexture(pRenderer, Resource::ICON_USER_SETTINGS, "./resources/gui/icons/icon_user_settings.png");
		LoadTexture(pRenderer, Resource::ICON_NEW_CHAT, "./resources/gui/icons/icon_new_chat.png");
		LoadTexture(pRenderer, Resource::ICON_CHECKMARK, "./resources/gui/icons/icon_checkmark.png");
		LoadTexture(pRenderer, Resource::ICON_SORTING, "./resources/gui/icons/icon_sorting.png");
		LoadTexture(pRenderer, Resource::ICON_FILTERING, "./resources/gui/icons/icon_filtering.png");
		LoadTexture(pRenderer, Resource::ICON_STAR, "./resources/gui/icons/icon_star.png");
		LoadTexture(pRenderer, Resource::ICON_UNSTAR, "./resources/gui/icons/icon_unstar.png");
		LoadTexture(pRenderer, Resource::ICON_HIDE, "./resources/gui/icons/icon_hide.png");
		LoadTexture(pRenderer, Resource::ICON_UNHIDE, "./resources/gui/icons/icon_unhide.png");
		LoadTexture(pRenderer, Resource::ICON_DELETE, "./resources/gui/icons/icon_delete.png");
		LoadTexture(pRenderer, Resource::ICON_EDIT, "./resources/gui/icons/icon_edit.png");
		LoadTexture(pRenderer, Resource::ICON_PLAY, "./resources/gui/icons/icon_play.png");
		LoadTexture(pRenderer, Resource::ICON_PAUSE, "./resources/gui/icons/icon_pause.png");
		LoadTexture(pRenderer, Resource::ICON_STOP, "./resources/gui/icons/icon_stop.png");
		LoadTexture(pRenderer, Resource::ICON_EJECT, "./resources/gui/icons/icon_eject.png");
		LoadTexture(pRenderer, Resource::ICON_SPINNER, "./resources/gui/icons/icon_spinner.png");
		LoadTexture(pRenderer, Resource::ICON_EXPAND_ARROW_RIGHT, "./resources/gui/icons/icon_expand_arrow.png");
		LoadTexture(pRenderer, Resource::ICON_EXPAND_ARROW_LEFT, "./resources/gui/icons/icon_collapse_arrow.png");
		LoadTexture(pRenderer, Resource::ICON_SAVE, "./resources/gui/icons/icon_save.png");
		LoadTexture(pRenderer, Resource::ICON_CROSS, "./resources/gui/icons/icon_cross.png");
		LoadTexture(pRenderer, Resource::ICON_DROPLIST_ARROW, "./resources/gui/icons/icon_droplist_arrow.png");

		LoadTexture(pRenderer, Resource::ICON_CHARACTER_EDIT_INFO, "./resources/gui/icons/icon_character_edit_info.png");
		LoadTexture(pRenderer, Resource::ICON_CHARACTER_EDIT_VOICE, "./resources/gui/icons/icon_character_edit_voice.png");
		LoadTexture(pRenderer, Resource::ICON_CHARACTER_EDIT_IMAGES, "./resources/gui/icons/icon_character_edit_images.png");
		LoadTexture(pRenderer, Resource::ICON_CHARACTER_EDIT_STORY, "./resources/gui/icons/icon_character_edit_story.png");
		LoadTexture(pRenderer, Resource::ICON_CHARACTER_EDIT_CONCEPTS, "./resources/gui/icons/icon_character_edit_concepts.png");
		LoadTexture(pRenderer, Resource::ICON_CHARACTER_EDIT_MEMORIES, "./resources/gui/icons/icon_character_edit_memories.png");
		LoadTexture(pRenderer, Resource::ICON_CHARACTER_EDIT_ABOUT, "./resources/gui/icons/icon_character_edit_about.png");
		LoadTexture(pRenderer, Resource::ICON_CHARACTER_EDIT_INFO_SMALL, "./resources/gui/icons/icon_character_edit_info_small.png");
		LoadTexture(pRenderer, Resource::ICON_CHARACTER_EDIT_VOICE_SMALL, "./resources/gui/icons/icon_character_edit_voice_small.png");
		LoadTexture(pRenderer, Resource::ICON_CHARACTER_EDIT_IMAGES_SMALL, "./resources/gui/icons/icon_character_edit_images_small.png");
		LoadTexture(pRenderer, Resource::ICON_CHARACTER_EDIT_STORY_SMALL, "./resources/gui/icons/icon_character_edit_story_small.png");
		LoadTexture(pRenderer, Resource::ICON_CHARACTER_EDIT_CONCEPTS_SMALL, "./resources/gui/icons/icon_character_edit_concepts_small.png");
		LoadTexture(pRenderer, Resource::ICON_CHARACTER_EDIT_MEMORIES_SMALL, "./resources/gui/icons/icon_character_edit_memories_small.png");
		LoadTexture(pRenderer, Resource::ICON_CHARACTER_EDIT_ABOUT_SMALL, "./resources/gui/icons/icon_character_edit_about_small.png");
		
		LoadTexture(pRenderer, Resource::ICON_BORDER_01, "./resources/gui/card/borders/icon_border_01.png");
		LoadTexture(pRenderer, Resource::ICON_BORDER_02, "./resources/gui/card/borders/icon_border_02.png");
		LoadTexture(pRenderer, Resource::ICON_BORDER_03, "./resources/gui/card/borders/icon_border_03.png");
		LoadTexture(pRenderer, Resource::ICON_BORDER_04, "./resources/gui/card/borders/icon_border_04.png");
		LoadTexture(pRenderer, Resource::ICON_BORDER_05, "./resources/gui/card/borders/icon_border_05.png");
		LoadTexture(pRenderer, Resource::ICON_BORDER_06, "./resources/gui/card/borders/icon_border_06.png");

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

		LoadTexture(pRenderer, Resource::SQUARE_BACKGROUND_DEFAULT, "./resources/gui/chat/square_bg_default.png");
		LoadTexture(pRenderer, Resource::PROFILE_DEFAULT_IMAGE, "./resources/gui/chat/default_portrait.png");
		
		LoadTexture(pRenderer, Resource::MASK_CARD, "./resources/gui/masks/mask_card_corners.png");
		LoadTexture(pRenderer, Resource::MASK_CIRCLE, "./resources/gui/masks/mask_circle256.png");
		LoadTexture(pRenderer, Resource::MASK_SMALL_PORTRAIT_48PX, "./resources/gui/masks/mask_small_portrait_48px.png");
		LoadTexture(pRenderer, Resource::MASK_SMALL_PORTRAIT_56PX, "./resources/gui/masks/mask_small_portrait_56px.png");
		
		LoadTexture(pRenderer, Resource::MASK_GRADIENT_EASE_IN_CUBIC_LEFT, "./resources/gui/masks/mask_gradient_ease_in_cubic_l.png");
		LoadTexture(pRenderer, Resource::MASK_GRADIENT_EASE_IN_CUBIC_RIGHT, "./resources/gui/masks/mask_gradient_ease_in_cubic_r.png");
		LoadTexture(pRenderer, Resource::MASK_GRADIENT_EASE_IN_OUT_SINE_LEFT, "./resources/gui/masks/mask_gradient_ease_in_out_sine_l.png");
		LoadTexture(pRenderer, Resource::MASK_GRADIENT_EASE_IN_OUT_SINE_RIGHT, "./resources/gui/masks/mask_gradient_ease_in_out_sine_r.png");

		LoadTexture(pRenderer, Resource::SLIDER_BAR_BG, "./resources/gui/slider_bar_bg.png");
		LoadTexture(pRenderer, Resource::SLIDER_BAR_BORDER, "./resources/gui/slider_bar_border.png");
		LoadTexture(pRenderer, Resource::SLIDER_THUMB_BG, "./resources/gui/slider_thumb_bg.png");
		LoadTexture(pRenderer, Resource::SLIDER_THUMB_BORDER, "./resources/gui/slider_thumb_border.png");
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

	fig::texture_ptr AppResources::GetUserProfileImage(fig::renderer_ptr pRenderer, const fig::user::UserProfile& profile)
	{
		auto itFind = _profileTextures.find(profile.id);
		if (itFind != _profileTextures.end())
			return itFind->second.get();

		fig::surface_ptr pSurface = nullptr;
		if (auto itSurf = _profileSurfaces.find(profile.id); itSurf != _profileSurfaces.end())
		{
			pSurface = itSurf->second.get();
		}
		else
		{
			// Load from disk
			if (auto result = AssetFileReader::ReadProfileFile(profile, std::format("{}.{}", Constants::Paths::ProfileImageFileName, Constants::Paths::ProfileImageFileExt)); result.has_value())
			{
				const auto& asset = result.value();
				auto width = asset.get_meta<uint16_t>(fig::io::MetaTag::ImageWidth).value_or(0);
				auto height = asset.get_meta<uint16_t>(fig::io::MetaTag::ImageHeight).value_or(0);
				auto format = static_cast<ImageFormat>(asset.get_meta<uint8_t>(fig::io::MetaTag::ImageFormat).value_or(0));

				if (auto image = CreateSurfaceFromBytes(width, height, format, asset.data); not image.empty())
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
				auto& texture = _profileTextures[profile.id] = fig::sdl::Texture::from_ptr(pTexture);
				return texture.get();
			}
		}

		return nullptr;
	}
}