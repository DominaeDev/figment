#pragma once

#include <SDL3/SDL.h>

struct TTF_TextEngine;
class Control;

struct AppState
{
    SDL_Window* pWindow;
    SDL_Renderer* pRenderer;
	Control* pTopFrame;
    Uint64 last_step;
	TTF_TextEngine* pTextEngine;
};