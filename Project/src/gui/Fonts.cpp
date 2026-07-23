#include <pch.h>
#include "gui/Fonts.h"

namespace fig::gui
{
	std::map<FontFace, const char*> s_FontFilenames {
		{ FontFace::Regular,	"./resources/fonts/Nunito-Regular.ttf" },
		{ FontFace::Italic,		"./resources/fonts/Nunito-Italic.ttf" },
		{ FontFace::Bold,		"./resources/fonts/Nunito-Bold.ttf" },
		{ FontFace::NunitoBold,	"./resources/fonts/Nunito-MediumItalic.ttf" },
		{ FontFace::CardHeader,	"./resources/fonts/Nunito-BoldItalic.ttf" },
	};

	std::list<const char*> s_FallbackFontFilenames {
		{"./fonts/KosugiMaru-Regular.ttf" },
		{"./fonts/NotoSansKR-Regular.ttf" },
		{"./fonts/NotoColorEmoji-Regular.ttf" },
	};

	std::map<FontFace, std::list<Fonts::font_face>> Fonts::s_Fonts = {};
	std::list<Fonts::font_face> Fonts::s_FallbackFonts = {};

	void Fonts::Init()
	{
		GetFont(FontFace::Regular, Constants::GUI::DefaultFontSize);
		GetFont(FontFace::Italic, Constants::GUI::DefaultFontSize);
		GetFont(FontFace::Bold, Constants::GUI::DefaultFontSize);
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

	std::list<fig::font_ptr> Fonts::GetFallbackFonts(double ptSize)
	{
		int iSize = (int)(ptSize * 10.0);

		std::list<fig::font_ptr> fonts;
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

	fig::font_ptr Fonts::LoadFont(FontFace face, const char* filename, double ptSize)
	{
		fig::font_ptr pFont;
		DEBUG_MEASURE_BEGIN(std::format("Load font [{} {}]", filename, ptSize));
		pFont = TTF_OpenFont(filename, (float)ptSize);
		if (pFont != nullptr)
		{
			TTF_SetFontHinting(pFont, TTF_HINTING_LIGHT_SUBPIXEL);
			TTF_SetFontKerning(pFont, true);
			s_Fonts[face].push_back(font_face { pFont, face, (int)(ptSize * 10.0) });

			auto fallbacks = GetFallbackFonts(ptSize);
			for (auto& pFallbackFont : fallbacks)
				TTF_AddFallbackFont(pFont, pFallbackFont);
		}
		DEBUG_MEASURE_END();
		return pFont;
	}

	fig::font_ptr Fonts::LoadFallbackFont(const char* filename, double ptSize)
	{
		fig::font_ptr pFont;
		DEBUG_MEASURE_BEGIN(std::format("Load font [{} {}]", filename, ptSize));
		pFont = TTF_OpenFont(filename, (float)ptSize);
		if (pFont != nullptr)
		{
			TTF_SetFontHinting(pFont, TTF_HINTING_LIGHT_SUBPIXEL);
			TTF_SetFontKerning(pFont, true);
			s_FallbackFonts.push_back(font_face { pFont, FontFace::Default, (int)(ptSize * 10.0) });
		}
		DEBUG_MEASURE_END();
		return pFont;
	}

	fig::font_ptr Fonts::GetFont(FontFace face, double ptSize)
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
}