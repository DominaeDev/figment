#include <pch.h>
#include "gui/ColorRef.h"
#include "gui/ColorTable.h"

namespace fig
{
	color_ref color_ref::nullref(&color_ref::null);

	color_ref::color_ref(fig::gui::Colour color) : _ptr { &fig::gui::_ColorTable.at(static_cast<size_t>(color)) }
	{
	}

	fig::color_ref color_ref::WithAlpha(uint8_t alpha) const noexcept
	{
		return fig::gui::custom_color(fig::color { _ptr->r, _ptr->g, _ptr->b, alpha });
	}
}