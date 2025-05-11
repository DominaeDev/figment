#pragma once

#include <SDL3_ttf/SDL_ttf.h>
#include <map>
#include <list>

enum class FontFace
{
	RobotoRegular,
	RobotoItalic,
	RobotoBold,

	NotoSansKR,
	NotoSansJP,
	Emoji,

	Default = RobotoRegular,
};

class Fonts
{
public:
	static void PreloadFonts();
	static void ReleaseFonts();

	static TTF_Font* GetFont(FontFace, double ptSize);

private:
	static TTF_Font* LoadFont(FontFace face, const char* filename, double ptSize);

private:

	struct Font
	{
		TTF_Font* _pFont;
		int size;
	};
	static std::map<FontFace, std::list<Font>> s_Fonts;
};
