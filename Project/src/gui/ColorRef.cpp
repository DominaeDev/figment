#include <pch.h>
#include "gui/ColorRef.h"
#include "gui/ColorTheme.h"

namespace fig
{
	const color_ref color_ref::nullref(&color_ref::null_value);

	color_ref::color_ref(fig::gui::Color color) : 
		_ptr { &fig::gui::ColorTheme::Get(color) }
	{
		int k = 0;
	}

	color_ref_with_alpha color_ref::WithAlpha(uint8_t alpha) const noexcept
	{
		return color_ref_with_alpha(*this, alpha);
	}
}