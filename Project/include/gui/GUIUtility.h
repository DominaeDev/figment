#ifndef GUI_UTILITY_H__
#define GUI_UTILITY_H__
#pragma once

#include "gui/GUITypes.h"

namespace fig::gui_util
{
	inline constexpr fig::gui::Rect expand_rect(const fig::gui::Rect& rect, int pixels)
	{
		return fig::gui::Rect { rect.x - pixels, rect.y - pixels, rect.w + pixels * 2, rect.h + pixels * 2 };
	}

	inline constexpr fig::gui::Rectf expand_rect(const fig::gui::Rectf& rect, float pixels)
	{
		return fig::gui::Rectf { rect.x - pixels, rect.y - pixels, rect.w + pixels * 2, rect.h + pixels * 2 };
	}

	inline constexpr fig::gui::Rect to_rect(fig::gui::Rectf rect)
	{
		return fig::gui::Rect {
			(int32_t)rect.x,
			(int32_t)rect.y,
			(int32_t)rect.w,
			(int32_t)rect.h
		};
	}

	inline constexpr fig::gui::Rectf to_rectf(fig::gui::Rect rect)
	{
		return fig::gui::Rectf {
			(float)rect.x,
			(float)rect.y,
			(float)rect.w,
			(float)rect.h
		};
	}

	bool is_defined(fig::gui::Color color);
	fig::gui::Color add_rgb(fig::gui::Color colorA, fig::gui::Color colorB);
	fig::gui::Color add_rgb(fig::gui::Color colorA, int value);
	fig::gui::Color add_rgb(fig::gui::Color colorA, float value);
	fig::gui::Color multiply_rgb(fig::gui::Color colorA, fig::gui::Color colorB);
	fig::gui::Color multiply_rgb(fig::gui::Color colorA, float value);
	fig::gui::Color with_alpha(fig::gui::Color color, Uint8 alpha);

	fig::gui::Color color_from_string(fig::string hex);
	void color_to_hsv(fig::gui::Color color, float& h, float& s, float& v);
	fig::gui::Color hsv_to_color(float h, float s, float v);
}

#endif