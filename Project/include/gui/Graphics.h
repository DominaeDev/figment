#pragma once

#include <SDL3/SDL.h>
#include <string>

struct SDL_Renderer;
struct SDL_Texture;
struct TTF_TextEngine;
class Control;
class Panel;
class LayoutElement;
class Sizer;

// SDL types
using Pointf = SDL_FPoint;
using Point = SDL_Point;
using Rectf = SDL_FRect;
using Rect = SDL_Rect;
using Colorf = SDL_FColor;
using Color = SDL_Color;
using Renderer = SDL_Renderer;
using Texture = SDL_Texture;
using Vertex = SDL_Vertex;
using Surface = SDL_Surface;

namespace gui_util
{
	inline constexpr Rect expand_rect(const Rect& rect, int pixels)
	{
		return Rect { rect.x - pixels, rect.y - pixels, rect.w + pixels * 2, rect.h + pixels * 2 };
	}

	inline constexpr Rectf expand_rect(const Rectf& rect, float pixels)
	{
		return Rectf { rect.x - pixels, rect.y - pixels, rect.w + pixels * 2, rect.h + pixels * 2 };
	}

	inline constexpr Rect to_rect(Rectf rect)
	{
		return Rect { 
			(int32_t)rect.x, 
			(int32_t)rect.y, 
			(int32_t)rect.w, 
			(int32_t)rect.h
		};
	}

	inline constexpr Rectf to_rectf(Rect rect)
	{
		return Rectf { 
			(float)rect.x, 
			(float)rect.y, 
			(float)rect.w, 
			(float)rect.h 
		};
	}
}