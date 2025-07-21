#pragma once

#include <SDL3/SDL.h>

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

extern Rectf expand_rect(const Rectf& rect, float pixels);
extern inline Rect toRect(Rectf rect);
extern inline Rectf toRectf(Rect rect);