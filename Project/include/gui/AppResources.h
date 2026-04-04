#pragma once

#include "gui/GUITypes.h"
#include <map>

namespace fig::user
{
	struct UserProfile;
}

namespace fig::gui
{
	enum class TextureType
	{
		BLANK,
		BORDER,
		LOGO_SMALL,
		ROUNDED_BACKGROUND,
		ROUNDED_BORDER,

		TEXTBOX_BG,
		TEXTBOX_BORDER,

		SPEECH_BUBBLE_LEFT_BG,
		SPEECH_BUBBLE_LEFT_BORDER,
		SPEECH_BUBBLE_CENTER_BG,
		SPEECH_BUBBLE_CENTER_BORDER,
		SPEECH_BUBBLE_RIGHT_BG,
		SPEECH_BUBBLE_RIGHT_BORDER,

		CARD_BORDER,
		CARD_BOTTOM_FADE,
		CARD_BOTTOM_FADE_SMALL,
		CARD_TAG_BG,
		CARD_ICON_CHAT_COUNTER,
		CARD_ICON_FAVORITE_OFF,
		CARD_ICON_FAVORITE_ON,

		CARD_BACKGROUND_DEFAULT,
		CARD_BACKGROUND_EMPTY,

		CARD_BORDER_STYLE_01,
		CARD_BORDER_STYLE_02,
		CARD_BORDER_STYLE_03,
		CARD_BORDER_STYLE_04,
		CARD_BORDER_STYLE_05,
		CARD_BORDER_STYLE_06,

		ICON_ERROR,
		ICON_SIDEBAR,
		ICON_SIDEBAR_COLLAPSE,
		ICON_SIDEBAR_EXPAND,
		ICON_SETTINGS,
		ICON_SEARCH,
		ICON_MENU,
		ICON_MENU_CHATS,
		ICON_MENU_CHARACTERS,
		ICON_MENU_SCENARIOS,
		ICON_GRID_SMALL,
		ICON_GRID_LARGE,
		ICON_TAG,
		ICON_LOGOUT,
		ICON_LOCK,
		ICON_ARROW_LEFT,
		ICON_ARROW_RIGHT,
		ICON_CHEVRON_LEFT,
		ICON_CHEVRON_RIGHT,
		ICON_HOME,

		PROFILE_DEFAULT_IMAGE,
		CIRCLE_MASK,

		MASK_CARD,
	};

	enum class MaskType
	{
		CARD_CORNER_MASK,
	};

	class AppResources
	{
	public:
		static void Init(Renderer* pRenderer);
		static void Release();
		static TexturePtr GetTexture(TextureType id);
		static SurfacePtr GetImage(TextureType id) noexcept;
		static MaskPtr GetMask(MaskType maskId);

		static TexturePtr GetUserProfileImage(Renderer* pRenderer, const fig::user::UserProfile& profile);

	private:
		static bool LoadTexture(Renderer* pRenderer, TextureType textureId, fig::path filename);
		static bool LoadTextureAndMaskCorners(Renderer* pRenderer, TextureType textureId, MaskType maskId, fig::path filename);
		static bool LoadMask(MaskType textureId, fig::path filename);

		static std::map<TextureType, fig::sdl::Surface> _surfaces;
		static std::map<TextureType, fig::sdl::Texture> _textures;
		static std::map<MaskType, Mask> _masks;
		static std::map<fig::uuid, fig::sdl::Surface> _profileSurfaces;
		static std::map<fig::uuid, fig::sdl::Texture> _profileTextures;
	};
}