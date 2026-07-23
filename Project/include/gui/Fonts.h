#pragma once

#include "GUITypes.h"

namespace fig::gui
{
	enum class FontFace
	{
		Regular,
		Italic,
		Bold,
		NunitoBold,
		CardHeader,
		CardSubheader = Regular,
		Default = Regular,
	};

	class Fonts
	{
	public:
		static void Init();
		static void ReleaseFonts();

		static fig::font_ptr GetFont(FontFace, double ptSize);

	private:
		static fig::font_ptr LoadFont(FontFace face, const char* filename, double ptSize);
		static std::list<fig::font_ptr> GetFallbackFonts(double ptSize);
		static fig::font_ptr LoadFallbackFont(const char* filename, double ptSize);

	private:
		struct font_face
		{
			fig::font_ptr pFont;
			FontFace face;
			int size;
		};
		static std::map<FontFace, std::list<font_face>> s_Fonts;
		static std::list<font_face> s_FallbackFonts;
	};
}