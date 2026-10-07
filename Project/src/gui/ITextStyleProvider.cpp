#include <pch.h>
#include "gui/ITextStyleProvider.h"

namespace fig::gui
{
	const TextStyle& ITextStyleProvider::GetTextStyle(TextStyleId styleId) const
	{
		assert(styleId < _styles.size());
		return _styles[styleId];
	}

	TextStyleId ITextStyleProvider::AddStyle(fig::color_ref fgColor) noexcept
	{
		_styles.emplace_back(TextStyle {
			.fgColor = fgColor,
		});
		return _styles.size() - 1uz;
	}
}