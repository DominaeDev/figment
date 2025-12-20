#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "Types.h"
#include "c_resource.h"

namespace fig::sdl
{
	using Window = stdex::c_resource<SDL_Window, SDL_CreateWindow, SDL_DestroyWindow>;
	using Renderer = stdex::c_resource<SDL_Renderer, SDL_CreateRenderer, SDL_DestroyRenderer>;
	using Texture = stdex::c_resource<SDL_Texture, SDL_CreateTexture, SDL_DestroyTexture>;
	using Surface = stdex::c_resource<SDL_Surface, SDL_CreateSurface, SDL_DestroySurface>;
	using Cursor = stdex::c_resource<SDL_Cursor, SDL_CreateCursor, SDL_DestroyCursor>;
	using TextEngine = stdex::c_resource<TTF_TextEngine, TTF_CreateRendererTextEngine, TTF_DestroyRendererTextEngine>;
}

namespace fig::gui
{
	// SDL types
	using Pointf = SDL_FPoint;
	using Point = SDL_Point;
	using Rectf = SDL_FRect;
	using Rect = SDL_Rect;
	using Colorf = SDL_FColor;
	using Color = SDL_Color;
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
}

namespace fig::gui_util
{
	inline constexpr fig::gui::Rect expand_rect(const fig::gui::Rect& rect, int pixels)
	{
		return fig::gui::Rect { rect.x - pixels, rect.y - pixels, rect.w + pixels * 2, rect.h + pixels * 2 };
	}

	inline constexpr fig::gui::Rectf expand_rect(const fig::gui::Rectf& rect, float pixels)
	{
		return fig::gui::Rectf { rect.x - pixels, rect.y - pixels, rect.w + pixels * 2, rect.h + pixels * 2 };
	}

	inline constexpr fig::gui::Rect to_rect(fig::gui::Rectf rect)
	{
		return fig::gui::Rect {
			(int32_t)rect.x, 
			(int32_t)rect.y, 
			(int32_t)rect.w, 
			(int32_t)rect.h
		};
	}

	inline constexpr fig::gui::Rectf to_rectf(fig::gui::Rect rect)
	{
		return fig::gui::Rectf {
			(float)rect.x, 
			(float)rect.y, 
			(float)rect.w, 
			(float)rect.h 
		};
	}
}