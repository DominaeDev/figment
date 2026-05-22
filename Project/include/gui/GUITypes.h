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
	using Cursor = stdex::c_resource<SDL_Cursor, SDL_CreateCursor, SDL_DestroyCursor>;
	using TextEngine = stdex::c_resource<TTF_TextEngine, TTF_CreateRendererTextEngine, TTF_DestroyRendererTextEngine>;

	using TextureRef = const std::reference_wrapper<Texture>;
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
	using Font = TTF_Font;

	using Renderer = SDL_Renderer;
	using Surface = SDL_Surface;
	using Texture = SDL_Texture;
	using Vertex = SDL_Vertex;
	using Colorf = SDL_FColor;

	using WindowPtr = SDL_Window*;
	using RendererPtr = SDL_Renderer*;
	using SurfacePtr = SDL_Surface*;
	using TexturePtr = SDL_Texture*;
	using VertexPtr = SDL_Vertex*;
	using TextEnginePtr = TTF_TextEngine*;

	using Coord = int32_t;

	struct Mask
	{
		std::vector<uint8_t> pixels;
		size_t width {};
		size_t height {};
		size_t pitch {};
	};

	using MaskPtr = const Mask*;

	struct ColorPair
	{
		Color foreground;
		Color background;
	};

	struct ButtonTheme
	{
		ColorPair defaultColor;
		ColorPair hoverColor;
		ColorPair pressedColor;
		ColorPair disabledColor;
	};

	enum class TextureType;	//! @hmm
	enum class MaskType; //! @hmm

	enum class ImageFormat : uint8_t
	{
		Undefined = 0x00,
		RGB24 = 0x03,	// SDL_PIXELFORMAT_RGB24
		RGBA32 = 0x04,	// SDL_PIXELFORMAT_RGBA8888
	};
}