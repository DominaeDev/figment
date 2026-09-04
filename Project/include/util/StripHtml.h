#pragma once

#include "Figment.h"

namespace fig
{
	bool contains_html(fig::string_view text) noexcept;
	fig::string strip_html(fig::string_view html) noexcept;
}