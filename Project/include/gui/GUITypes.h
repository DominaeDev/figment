#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "Types.h"
#include "c_resource.h"
#include "gui/GUIColors.h"

namespace fig::sdl
{
	using Window = stdex::c_resource<SDL_Window, SDL_CreateWindow, SDL_DestroyWindow>;
	using Renderer = stdex::c_resource<SDL_Renderer, SDL_CreateRenderer, SDL_DestroyRenderer>;
	using Texture = stdex::c_resource<SDL_Texture, SDL_CreateTexture, SDL_DestroyTexture>;
	using Surface = stdex::c_resource<SDL_Surface, SDL_CreateSurface, SDL_DestroySurface>;
	using Cursor = stdex::c_resource<SDL_Cursor, SDL_CreateCursor, SDL_DestroyCursor>;
	using TextEngine = stdex::c_resource<TTF_TextEngine, TTF_CreateRendererTextEngine, TTF_DestroyRendererTextEngine>;
}

namespace fig
{
	template <typename T, typename P = T::pointer>
	concept IsCResource = requires(T type, P p)
	{
		type.reset(p);
	};

	template <IsCResource T, typename P = T::pointer>
	T make_cresource(P ptr)
	{
		T surface;
		surface.reset(ptr);
		return surface;
	};
}

namespace fig::gui
{
	// SDL types
	using Pointf = SDL_FPoint;
	using Point = SDL_Point;
	using Rectf = SDL_FRect;
	using Rect = SDL_Rect;
	using Event = SDL_Event;

	using Renderer = SDL_Renderer;
	using Surface = SDL_Surface;
	using Texture = SDL_Texture;
	using Vertex = SDL_Vertex;

	using WindowPtr = SDL_Window*;
	using RendererPtr = SDL_Renderer*;
	using SurfacePtr = SDL_Surface*;
	using TexturePtr = SDL_Texture*;
	using VertexPtr = SDL_Vertex*;
	using TextEnginePtr = TTF_TextEngine*;

	enum class TextureType
	{
		BLANK,
		BORDER,
		LOGO_SMALL,

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
		ICON_MENU,
	};

	enum class MaskType
	{
		CARD_CORNER_MASK,
	};

	struct Mask
	{
		std::vector<uint8_t> pixels;
		size_t width;
		size_t height;
		size_t pitch;
	};

	using MaskPtr = const Mask*;
}