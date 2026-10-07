#pragma once

#include "Figment.h"

namespace fig::gui
{
	using TextStyleId = size_t;

	struct StyleSpan
	{
		int32_t position;
		int32_t length;
		TextStyleId styleId;
	};

	struct TextStyle
	{
		fig::color_ref fgColor;
		// ...
	};

	class ITextStyleProvider
	{
	public:
		virtual ~ITextStyleProvider() {}
		const TextStyle& GetTextStyle(TextStyleId styleId) const;

	protected:
		friend class TextBase;
		virtual std::vector<StyleSpan> GetStyles(fig::string_view text, size_t offset = 0uz) = 0;

		TextStyleId AddStyle(fig::color_ref fgColor) noexcept;

	private:
		std::vector<TextStyle> _styles;
	};
}