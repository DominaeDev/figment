#pragma once

#include "Fonts.h"

struct TTF_TextEngine;
struct TTF_Font;

class Text
{
public:
	static TTF_TextEngine* InitEngine(SDL_Renderer* pRenderer);
	static TTF_TextEngine* GetEngine() { return _pEngine; }


//	static void DrawText(const char* text, int x, int y, TTF_Font* pFont, SDL_Color foreground, SDL_Color background);

private:
	static TTF_TextEngine* _pEngine;
};