#ifndef GUI_UTILITY_H__
#define GUI_UTILITY_H__
#pragma once

#include "gui/GUITypes.h"

namespace fig::gui::util
{
	inline constexpr Rect expand_rect(const Rect& rect, int pixels)
	{
		return Rect { rect.x - pixels, rect.y - pixels, rect.w + pixels * 2, rect.h + pixels * 2 };
	}

	inline constexpr Rectf expand_rect(const Rectf& rect, float pixels)
	{
		return Rectf { rect.x - pixels, rect.y - pixels, rect.w + pixels * 2, rect.h + pixels * 2 };
	}

	inline constexpr Rect to_rect(const Rectf& rect)
	{
		return Rect {
			(int32_t)rect.x,
			(int32_t)rect.y,
			(int32_t)rect.w,
			(int32_t)rect.h
		};
	}

	inline constexpr Rectf to_rectf(const Rect& rect)
	{
		return Rectf {
			(float)rect.x,
			(float)rect.y,
			(float)rect.w,
			(float)rect.h
		};
	}

	inline constexpr bool is_inside(const Rect& rect, int x, int y, int expand = 0)
	{
		return x - expand >= rect.x and x + expand < rect.x + rect.w
			and y - expand >= rect.y and y + expand < rect.y + rect.h;
	}

	inline constexpr bool is_inside(const Rectf& rect, float x, float y, float expand = 0.0f)
	{
		return x - expand >= rect.x and x + expand< rect.x + rect.w
			and y - expand >= rect.y and y + expand< rect.y + rect.h;
	}

	inline constexpr bool is_inside(const Rect& rect, const Point& p, int expand = 0)
	{
		return p.x - expand >= rect.x and p.x + expand< rect.x + rect.w
			and p.y - expand >= rect.y and p.y + expand< rect.y + rect.h;
	}

	inline constexpr bool is_inside(const Rectf& rect, const Pointf& p, float expand = 0.0f)
	{
		return p.x - expand >= rect.x and p.x + expand< rect.x + rect.w
			and p.y - expand >= rect.y and p.y + expand< rect.y + rect.h;
	}

	bool is_defined(Color color);
	Color add_rgb(Color colorA, Color colorB);
	Color add_rgb(Color colorA, int value);
	Color add_rgb(Color colorA, float value);
	Color multiply_rgb(Color colorA, Color colorB);
	Color multiply_rgb(Color colorA, float value);
	Color with_alpha(Color color, Uint8 alpha);
	Color with_alpha(Color color, float alpha);

	Color color_from_string(fig::string hex);
	void color_to_hsv(Color color, float& h, float& s, float& v);
	Color hsv_to_color(float h, float s, float v);

	enum class ImageFit {
		None,
		Stretch,
		Inside,
		Outside,
		Portrait,
	};

	std::optional<fig::sdl::Surface> LoadImage(fig::path filename);
	fig::sdl::Surface LoadImageFromMemory(fig::byte_span data);
	fig::sdl::Surface LoadAndResizeImage(fig::path filename, int32_t width, int32_t height, ImageFit fit = ImageFit::None);
	fig::sdl::Surface ScaleSurface(const fig::sdl::Surface& surface, int32_t width, int32_t height, ImageFit fit = ImageFit::None);
	fig::sdl::Surface CreateCoverImage(const fig::sdl::Surface& surface, bool bAlpha);

	enum class CornerStyle {
		Card,
	};
	bool MaskCorners(fig::sdl::Surface& surface, MaskType style);
	void AlphaToMask(fig::path filename);

	Point MeasureText(Font& font, const fig::string& text);
	int MeasureFontHeight(Font& font);
}

#endif