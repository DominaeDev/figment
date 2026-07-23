#pragma once

#include "gui/GUITypes.h"
#include <map>

namespace fig::user
{
	struct UserProfile;
}

namespace fig
{
	enum class Resource
	{
		NONE,

		BLANK,
		BORDER,
		LOGO_SMALL,
		ROUNDED_BACKGROUND_6PX,
		ROUNDED_BORDER_6PX,
		ROUNDED_BACKGROUND_10PX,
		ROUNDED_BORDER_10PX,
		SUBMENU_ARROW,
		BACKGROUND_CIRCLE_48PX,

		TEXTBOX_BG,
		TEXTBOX_BORDER,

		SPEECH_BUBBLE_LEFT_BG,
		SPEECH_BUBBLE_LEFT_BORDER,
		SPEECH_BUBBLE_CENTER_BG,
		SPEECH_BUBBLE_CENTER_BORDER,
		SPEECH_BUBBLE_RIGHT_BG,
		SPEECH_BUBBLE_RIGHT_BORDER,

		CARD_FILL,
		CARD_BORDER,
		CARD_BOTTOM_FADE,
		CARD_BOTTOM_FADE_SMALL,
		CARD_TAG_BG,
		CARD_ICON_CHAT_COUNTER,
		CARD_ICON_STAR,
		CARD_ICON_STAR_SMALL,

		CARD_BACKGROUND_DEFAULT,
		CARD_BACKGROUND_EMPTY,

		CARD_BORDER_STYLE_01,
		CARD_BORDER_STYLE_02,
		CARD_BORDER_STYLE_03,
		CARD_BORDER_STYLE_04,
		CARD_BORDER_STYLE_05,
		CARD_BORDER_STYLE_06,

		SQUARE_BACKGROUND_DEFAULT,

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
		ICON_MENU_WORLDS,
		ICON_MENU_MODELS,
		ICON_MENU_CHATS_SMALL,
		ICON_MENU_CHARACTERS_SMALL,
		ICON_MENU_SCENARIOS_SMALL,
		ICON_MENU_WORLDS_SMALL,
		ICON_MENU_MODELS_SMALL,
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
		ICON_USER_SETTINGS,
		ICON_NEW_CHAT,
		ICON_CHECKMARK,
		ICON_SORTING,
		ICON_FILTERING,
		ICON_STAR,
		ICON_UNSTAR,
		ICON_HIDE,
		ICON_UNHIDE,
		ICON_DELETE,
		ICON_EDIT,
		ICON_EXPAND_ARROW_RIGHT,
		ICON_EXPAND_ARROW_LEFT,

		ICON_PLAY,
		ICON_PAUSE,
		ICON_STOP,
		ICON_EJECT,
		ICON_SPINNER,
		
		ICON_BORDER_01,
		ICON_BORDER_02,
		ICON_BORDER_03,
		ICON_BORDER_04,
		ICON_BORDER_05,
		ICON_BORDER_06,

		PROFILE_DEFAULT_IMAGE,

		MASK_CARD,
		MASK_CIRCLE,
		MASK_SMALL_PORTRAIT_48PX,
		MASK_SMALL_PORTRAIT_56PX,

		MASK_GRADIENT_EASE_IN_CUBIC_LEFT,
		MASK_GRADIENT_EASE_IN_CUBIC_RIGHT,
		MASK_GRADIENT_EASE_IN_OUT_SINE_LEFT,
		MASK_GRADIENT_EASE_IN_OUT_SINE_RIGHT,
	};

	enum class MaskType
	{
		CARD_CORNER_MASK,
	};

	struct Mask
	{
		std::vector<uint8_t> pixels;
		size_t width {};
		size_t height {};
		size_t pitch {};
	};

	using MaskPtr = const Mask*;

	class AppResources
	{
	public:
		static void Init(fig::renderer_ptr pRenderer);
		static void Release();
		static fig::texture_ptr GetTexture(Resource id);
		static fig::surface_ptr GetImage(Resource id) noexcept;
		static MaskPtr GetMask(MaskType maskId);

		static fig::texture_ptr GetUserProfileImage(fig::renderer_ptr pRenderer, const fig::user::UserProfile& profile);

	private:
		static bool LoadTexture(fig::renderer_ptr pRenderer, Resource textureId, fig::path filename);
		static bool LoadTextureAndMaskCorners(fig::renderer_ptr pRenderer, Resource textureId, MaskType maskId, fig::path filename);
		static bool LoadMask(MaskType textureId, fig::path filename);

		static std::map<Resource, fig::sdl::Surface> _surfaces;
		static std::map<Resource, fig::sdl::Texture> _textures;
		static std::map<MaskType, Mask> _masks;
		static std::map<fig::uuid, fig::sdl::Surface> _profileSurfaces;
		static std::map<fig::uuid, fig::sdl::Texture> _profileTextures;
	};
}