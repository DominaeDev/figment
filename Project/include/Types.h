#pragma once
#include <xstring>
#include <SDL3/SDL.h>

typedef std::u8string String;

class Control;

struct AppState
{
    SDL_Window* pWindow;
    SDL_Renderer* pRenderer;
	Control* pTopFrame;
    Uint64 last_step;
};