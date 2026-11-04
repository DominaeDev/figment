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
		THICK_ROUNDED_BORDER_6PX,
		ROUNDED_BACKGROUND_10PX,
		ROUNDED_BORDER_10PX,
		THICK_ROUNDED_BORDER_10PX,
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
		CARD_COVER_CHARACTER,
		CARD_COVER_STORY,

		CARD_BORDER_STYLE_01,
		CARD_BORDER_STYLE_02,
		CARD_BORDER_STYLE_03,
		CARD_BORDER_STYLE_04,
		CARD_BORDER_STYLE_05,
		CARD_BORDER_STYLE_06,


		SQUARE_BACKGROUND_DEFAULT,

		PROFILE_DEFAULT_IMAGE,

		MASK_CARD,
		MASK_CIRCLE,
		MASK_SMALL_PORTRAIT_48PX,
		MASK_SMALL_PORTRAIT_56PX,
		MASK_THUMBNAIL_PORTRAIT,
		MASK_THUMBNAIL_LANDSCAPE,

		MASK_GRADIENT_EASE_IN_CUBIC_LEFT,
		MASK_GRADIENT_EASE_IN_CUBIC_RIGHT,
		MASK_GRADIENT_EASE_IN_OUT_SINE_LEFT,
		MASK_GRADIENT_EASE_IN_OUT_SINE_RIGHT,

		SLIDER_BAR_BG,
		SLIDER_BAR_BORDER,
		SLIDER_THUMB_BG,
		SLIDER_THUMB_BORDER,

		HORIZONTAL_BAR,
		HORIZONTAL_BAR_SMALL,
		VERTICAL_BAR_SMALL,
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

	private:
		static bool LoadTexture(fig::renderer_ptr pRenderer, Resource textureId, fig::path filename);
		static bool LoadTextureAndMaskCorners(fig::renderer_ptr pRenderer, Resource textureId, MaskType maskId, fig::path filename);
		static bool LoadMask(MaskType textureId, fig::path filename);

		static std::map<Resource, fig::sdl::Surface> _surfaces;
		static std::map<Resource, fig::sdl::Texture> _textures;
		static std::map<MaskType, Mask> _masks;
	};
}