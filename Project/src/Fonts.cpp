#include "Fonts.h"
#include "Constants.h"

std::map<FontFace, const char*> s_FontFaces {
	{ FontFace::RobotoRegular,	"./fonts/Notosans.ttf" },
	{ FontFace::RobotoItalic,	"./fonts/Roboto-Italic.ttf" },
	{ FontFace::RobotoBold,		"./fonts/Roboto-Medium.ttf" },
	{ FontFace::NotoSansJP,		"./fonts/NotoSansJP-Regular.ttf" },
	{ FontFace::NotoSansKR,		"./fonts/NotoSansKR-Regular.ttf" },
	{ FontFace::Emoji,			"./fonts/NotoColorEmoji-Regular.ttf" },
};

std::map<FontFace, std::list<Fonts::Font>> Fonts::s_Fonts = {};

void Fonts::PreloadFonts()
{
	GetFont(FontFace::RobotoRegular, Constants::DefaultFontSize);
	GetFont(FontFace::RobotoItalic, Constants::DefaultFontSize);
	GetFont(FontFace::RobotoBold, Constants::DefaultFontSize);
}

void Fonts::ReleaseFonts()
{
	for (auto& kvp : s_Fonts)
	{
		for (auto& font : kvp.second)
			TTF_CloseFont(font._pFont);
	}
	s_Fonts.clear();
}

TTF_Font* Fonts::LoadFont(FontFace face, const char* filename, double ptSize)
{
	TTF_Font* pFont = TTF_OpenFont(filename, (float)ptSize);
	if (pFont != nullptr)
	{
		TTF_SetFontHinting(pFont, TTF_HINTING_LIGHT_SUBPIXEL);
		TTF_SetFontKerning(pFont, true);
		s_Fonts[face].push_back(Font { pFont, (int)(ptSize * 10.0) });
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
				return font._pFont;
		}
	}

	return LoadFont(face, s_FontFaces[face], ptSize);
}