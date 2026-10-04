#pragma once

#include <algorithm>

namespace fig
{
	inline constexpr float ease_in_quad(float t) noexcept
	{
		t = std::clamp(t, 0.0f, 1.0f);
		return t * t;
	}

	inline constexpr float ease_in_cubic(float t) noexcept
	{
		t = std::clamp(t, 0.0f, 1.0f);
		return t * t * t;
	}
	inline constexpr float ease_out_quad(float t) noexcept
	{
		t = 1.0f - std::clamp(t, 0.0f, 1.0f);
		return 1.0f - t * t;
	}

	inline constexpr float ease_out_cubic(float t) noexcept
	{
		t = 1.0f - std::clamp(t, 0.0f, 1.0f);
		return 1.0f - t * t * t;
	}

}