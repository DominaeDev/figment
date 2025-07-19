#pragma once

#include <SDL3/SDL.h>

struct TTF_TextEngine;
struct SDL_Renderer;
struct SDL_Texture;

typedef SDL_FPoint Point;
typedef SDL_FRect Rect;

class Control;
class LayoutElement;
class Sizer;

extern SDL_FRect Rect_Expand(const SDL_FRect& rect, float pixels);