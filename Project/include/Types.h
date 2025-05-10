#pragma once
#include <xstring>
#include <SDL3/SDL.h>

typedef std::u8string String;

class Frame;

struct AppState
{
    SDL_Window* pWindow;
    SDL_Renderer* pRenderer;
	Frame* pTopFrame;
    Uint64 last_step;
};