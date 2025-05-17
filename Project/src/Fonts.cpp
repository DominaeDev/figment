#include "Fonts.h"
#include "Constants.h"
#include <vector>

std::map<FontFace, const char*> s_FontFilenames {
	{ FontFace::Regular,	"./fonts/Notosans.ttf" },
	{ FontFace::Italic,	"./fonts/Roboto-Italic.ttf" },
	{ FontFace::Bold,		"./fonts/Roboto-Medium.ttf" },
};

std::list<const char*> s_FallbackFontFilenames {
	{"./fonts/NotoSansJP-Regular.ttf" },
	{"./fonts/NotoSansKR-Regular.ttf" },
	{"./fonts/NotoColorEmoji-Regular.ttf" },
};

std::map<FontFace, std::list<Fonts::Font>> Fonts::s_Fonts = {};
std::list<Fonts::Font> Fonts::s_FallbackFonts = {};

void Fonts::Init()
{
	GetFont(FontFace::Regular, Constants::DefaultFontSize);
	GetFont(FontFace::Italic, Constants::DefaultFontSize);
	GetFont(FontFace::Bold, Constants::DefaultFontSize);
}

void Fonts::ReleaseFonts()
{
	for (auto& kvp : s_Fonts)
	{
		for (auto& font : kvp.second)
			TTF_CloseFont(font.pFont);
	}
	for (auto& font : s_FallbackFonts)
		TTF_CloseFont(font.pFont);
	s_Fonts.clear();
}

std::list<TTF_Font*> Fonts::GetFallbackFonts(double ptSize)
{
	int iSize = (int)(ptSize * 10.0);

	std::list<TTF_Font*> fonts;
	for (auto& font : s_FallbackFonts)
	{
		if (font.size == iSize)
			fonts.push_back(font.pFont);
	}

	if (fonts.size() == 0)
	{
		for(auto& fn : s_FallbackFontFilenames)
			fonts.push_back(LoadFallbackFont(fn, ptSize));
	}
	return fonts;
}

TTF_Font* Fonts::LoadFont(FontFace face, const char* filename, double ptSize)
{
	TTF_Font* pFont = TTF_OpenFont(filename, (float)ptSize);
	if (pFont != nullptr)
	{
		TTF_SetFontHinting(pFont, TTF_HINTING_LIGHT_SUBPIXEL);
		TTF_SetFontKerning(pFont, true);
		s_Fonts[face].push_back(Font { pFont, face, (int)(ptSize * 10.0) });

		auto fallbacks = GetFallbackFonts(ptSize);
		for (auto& pFallbackFont : fallbacks)
			TTF_AddFallbackFont(pFont, pFallbackFont);
		return pFont;
	}
	return nullptr;
}

TTF_Font* Fonts::LoadFallbackFont(const char* filename, double ptSize)
{
	TTF_Font* pFont = TTF_OpenFont(filename, (float)ptSize);
	if (pFont != nullptr)
	{
		TTF_SetFontHinting(pFont, TTF_HINTING_LIGHT_SUBPIXEL);
		TTF_SetFontKerning(pFont, true);
		s_FallbackFonts.push_back(Font { pFont, FontFace::Default, (int)(ptSize * 10.0) });
		return pFont;
	}
	return nullptr;
}

TTF_Font* Fonts::GetFont(FontFace face, double ptSize)
{
	int iSize = (int)(ptSize * 10.0);
	auto itFind = s_Fonts.find(face);
	if (itFind != std::cend(s_Fonts))
	{
		for (auto font : itFind->second)
		{
			if (font.size == iSize)
				return font.pFont;
		}
	}

	return LoadFont(face, s_FontFilenames[face], ptSize);
}
