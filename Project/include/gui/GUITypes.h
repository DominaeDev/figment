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
}