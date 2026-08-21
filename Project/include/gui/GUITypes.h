#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "Figment.h"
#include "gui/GUIColor.h"
#include "c_resource.h"

namespace fig::sdl
{
	using Window = stdex::c_resource<SDL_Window, SDL_CreateWindow, SDL_DestroyWindow>;
	using Renderer = stdex::c_resource<SDL_Renderer, SDL_CreateRenderer, SDL_DestroyRenderer>;
	using Texture = stdex::c_resource<SDL_Texture, SDL_CreateTexture, SDL_DestroyTexture>;
	using Surface = stdex::c_resource<SDL_Surface, SDL_CreateSurface, SDL_DestroySurface>;
	using Cursor = stdex::c_resource<SDL_Cursor, SDL_CreateSystemCursor, SDL_DestroyCursor>;
	using TextEngine = stdex::c_resource<TTF_TextEngine, TTF_CreateRendererTextEngine, TTF_DestroyRendererTextEngine>;
	using AudioStream = stdex::c_resource<SDL_AudioStream, SDL_OpenAudioDeviceStream, SDL_DestroyAudioStream>;
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

namespace fig
{
	using point = SDL_Point;
	using pointf = SDL_FPoint;
	using rect = SDL_Rect;
	using rectf = SDL_FRect;
	using vertex = SDL_Vertex;
	using event = SDL_Event;
	using font = TTF_Font;

	using window_ptr = fig::observer_ptr<SDL_Window>;
	using renderer_ptr = fig::observer_ptr<SDL_Renderer>;
	using surface_ptr = fig::observer_ptr<SDL_Surface>;
	using texture_ptr = fig::observer_ptr<SDL_Texture>;
	using text_engine_ptr = fig::observer_ptr<TTF_TextEngine>;
	using font_ptr = fig::observer_ptr<TTF_Font>;

	using coord = int32_t;
	using corners = std::array<coord, 4>;

	struct ButtonTheme
	{
		fig::color_pair defaultColor;
		fig::color_pair hoverColor;
		fig::color_pair pressedColor;
		fig::color_pair disabledColor;
	};

	enum class Resource;
	enum class MaskType;

	enum class ImageFormat : uint8_t
	{
		Undefined = 0x00,
		RGB24 = 0x03,	// SDL_PIXELFORMAT_RGB24
		RGBA32 = 0x04,	// SDL_PIXELFORMAT_RGBA8888
	};

	enum class Direction
	{
		North = 1 << 0,
		East = 1 << 1,
		South = 1 << 2,
		West = 1 << 3,
	};
	using Directions = EnumFlags<Direction>;

	using cursor = int32_t;

	namespace Cursor
	{
		constexpr cursor Default			= SDL_SYSTEM_CURSOR_DEFAULT;
		constexpr cursor Caret				= SDL_SYSTEM_CURSOR_TEXT;
		constexpr cursor ResizeHorizontal	= SDL_SYSTEM_CURSOR_EW_RESIZE;
		constexpr cursor ResizeVertical		= SDL_SYSTEM_CURSOR_NS_RESIZE;
	};

	using ToggleDelegate = std::function<void(bool)>;
	enum class ToggleBehavior { Default, Radio };

	using ListItemSelectedDelegate = std::function<void(int32_t)>;
}

namespace fig::gui
{
	class Control;
	class Sizer;
	class Window;
	class Image;
	class TextInput;
}