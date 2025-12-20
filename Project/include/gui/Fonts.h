#pragma once

#include <SDL3_ttf/SDL_ttf.h>
#include <map>
#include <list>

namespace fig::gui
{
	enum class FontFace
	{
		Regular,
		Italic,
		Bold,
		NunitoBold,
		Default = Regular,
	};

	class Fonts
	{
	public:
		static void Init();
		static void ReleaseFonts();

		static TTF_Font* GetFont(FontFace, double ptSize);

	private:
		static TTF_Font* LoadFont(FontFace face, const char* filename, double ptSize);
		static std::list<TTF_Font*> GetFallbackFonts(double ptSize);
		static TTF_Font* LoadFallbackFont(const char* filename, double ptSize);

	private:

		struct Font
		{
			TTF_Font* pFont;
			FontFace face;
			int size;
		};
		static std::map<FontFace, std::list<Font>> s_Fonts;

		static std::list<Font> s_FallbackFonts;
	};
}