export module GUI.Text.Fonts;

import Common;
import GUI.GraphicTypes;
import <SDL3_ttf/SDL_ttf.h>;

export enum class FontFace
{
	Regular,
	Italic,
	Bold,
	NunitoBold,
	Default = Regular,
};

std::map<FontFace, const char*> s_FontFilenames {
	{ FontFace::Regular,	"./resources/fonts/Nunito-Regular.ttf" },
	{ FontFace::Italic,		"./resources/fonts/Nunito-Italic.ttf" },
	{ FontFace::Bold,		"./resources/fonts/Roboto-Medium.ttf" },
	{ FontFace::NunitoBold,	"./resources/fonts/Nunito-MediumItalic.ttf" },
};

std::list<const char*> s_FallbackFontFilenames {
	{"./fonts/KosugiMaru-Regular.ttf" },
	{"./fonts/NotoSansKR-Regular.ttf" },
	{"./fonts/NotoColorEmoji-Regular.ttf" },
};

struct Font
{
	TTF_Font* pFont;
	FontFace face;
	int size;
};

export class Fonts
{
public:
	static void Init()
	{
		GetFont(FontFace::Regular, Constants::GUI::DefaultFontSize);
		GetFont(FontFace::Italic, Constants::GUI::DefaultFontSize);
		GetFont(FontFace::Bold, Constants::GUI::DefaultFontSize);
	}

	static void ReleaseFonts()
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

	static TTF_Font* GetFont(FontFace face, double ptSize)
	{
		int iSize = (int)(ptSize * 10.0);
		auto itFind = s_Fonts.find(face);
		if (itFind != s_Fonts.cend())
		{
			for (auto font : itFind->second)
			{
				if (font.size == iSize)
					return font.pFont;
			}
		}

		return LoadFont(face, s_FontFilenames[face], ptSize);
	}

private:
	static std::list<TTF_Font*> GetFallbackFonts(double ptSize)
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
			for (auto& fn : s_FallbackFontFilenames)
				fonts.push_back(LoadFallbackFont(fn, ptSize));
		}
		return fonts;
	}

	static TTF_Font* LoadFont(FontFace face, const char* filename, double ptSize)
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

	static TTF_Font* LoadFallbackFont(const char* filename, double ptSize)
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

private:
	static std::map<FontFace, std::list<Font>> s_Fonts;
	static std::list<Font> s_FallbackFonts;
};

std::map<FontFace, std::list<Font>> Fonts::s_Fonts = {};
std::list<Font> Fonts::s_FallbackFonts = {};

