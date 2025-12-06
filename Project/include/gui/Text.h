#ifndef TEXT_H__
#define TEXT_H__

#pragma once

#include "Fonts.h"
#include "Graphics.h"

struct TTF_TextEngine;
struct TTF_Font;

class Text
{
public:
	static TTF_TextEngine* InitEngine(Renderer* pRenderer);
	static TTF_TextEngine* GetEngine() { return _pEngine; }

private:
	static TTF_TextEngine* _pEngine;
};

#endif