export module Color;

import Types;
import Utility;
export import Graphics;

export namespace Colors
{
	inline constexpr Color White							{ 0xFF, 0xFF, 0xFF, 0xFF };
	inline constexpr Color Black							{ 0x00, 0x00, 0x00, 0xFF };
	inline constexpr Color DarkGray							{ 0x64, 0x64, 0x64, 0xFF };
	inline constexpr Color Transparent						{ 0xFF, 0xFF, 0xFF, 0x00 };
	inline constexpr Color Debug							{ 0xC0, 0x00, 0xC0, 0xFF };
	inline constexpr Color TextForeground					{ 0x00, 0x00, 0x00, 0xFF };
	inline constexpr Color TextSelectionForeground			{ 0xFF, 0xFF, 0xFF, 0xFF };
	inline constexpr Color TextSelectionBackground			{ 0x99, 0xC9, 0xEF, 0xFF };
	inline constexpr Color AppBackground					{ 0xfa, 0xf9, 0xf5, 255 };
	inline constexpr Color ChatBackground					{ 0xfa, 0xf9, 0xf5, 255 };
	inline constexpr Color MessageBorderDefault				{ 0x9f, 0x9f, 0x9f, 0xff };
	inline constexpr Color MessageBackgroundDefault			{ 0xf4, 0xf4, 0xf4, 0xff };
	inline constexpr Color MessageBorderBlue				{ 0x4d, 0xa1, 0xc1, 0xff };
	inline constexpr Color MessageBackgroundBlue			{ 0xf2, 0xfb, 0xff, 0xff };
	inline constexpr Color MessageBorderPink				{ 0xef, 0x76, 0xbd, 0xff };
	inline constexpr Color MessageBackgroundPink			{ 0xff, 0xf3, 0xf9, 0xff };
	inline constexpr Color MessageBorderGreen				{ 0x50, 0xe4, 0x33, 0xff };
	inline constexpr Color MessageBackgroundGreen			{ 0xea, 0xff, 0xe9, 0xff };
	inline constexpr Color MessageBorderYellow				{ 0xe4, 0xc5, 0x33, 0xff };
	inline constexpr Color MessageBackgroundYellow			{ 0xff, 0xfc, 0xea, 0xff };
	inline constexpr Color MessageBorderRed					{ 0xd5, 0x2b, 0x2b, 0xff };
	inline constexpr Color MessageBackgroundRed				{ 0xff, 0xee, 0xee, 0xff };
	inline constexpr Color MessageBorderTeal				{ 0x4d, 0xc1, 0xba, 0xff };
	inline constexpr Color MessageBackgroundTeal			{ 0xee, 0xff, 0xfc, 0xff };
	inline constexpr Color MessageBorderPurple				{ 0xb2, 0x5c, 0xe1, 0xff };
	inline constexpr Color MessageBackgroundPurple			{ 0xfc, 0xf3, 0xff, 0xff };
	inline constexpr Color MessageBorderBrown				{ 0xc0, 0x7c, 0x4c, 0xff };
	inline constexpr Color MessageBackgroundBrown			{ 0xff, 0xf9, 0xea, 0xff };
	inline constexpr Color MessageBorderNavy				{ 0x4d, 0x55, 0xc1, 0xff };
	inline constexpr Color MessageBackgroundNavy			{ 0xf2, 0xfa, 0xff, 0xff };

	inline constexpr Color DefaultUserMessageBorder = Colors::MessageBorderBlue;
	inline constexpr Color DefaultUserMessageBackground = Colors::MessageBackgroundBlue;

	inline constexpr std::array<Color, 8> DefaultBotMessageBorders {
		MessageBorderPink,
		MessageBorderGreen,
		MessageBorderYellow,
		MessageBorderRed,
		MessageBorderTeal,
		MessageBorderPurple,
		MessageBorderBrown,
		MessageBorderNavy,
	};

	inline constexpr std::array<Color, 8> DefaultBotMessageBackgrounds {
		MessageBackgroundPink,
		MessageBackgroundGreen,
		MessageBackgroundYellow,
		MessageBackgroundRed,
		MessageBackgroundTeal,
		MessageBackgroundPurple,
		MessageBackgroundBrown,
		MessageBackgroundNavy,
	};
}

export struct color_util
{
	static bool is_defined(Color color);
	static Colorf to_colorf(Color color);
	static Color to_color(Colorf color);

	static Color add_rgb(Color colorA, Color colorB);
	static Color add_rgb(Color colorA, int value);
	static Color add_rgb(Color colorA, float value);
	static Color multiply_rgb(Color colorA, Color colorB);
	static Color multiply_rgb(Color colorA, float value);
	static Color with_alpha(Color color, Uint8 alpha);

	static Color color_from_string(std::string hex);
	static void color_to_hsv(Color color, float& h, float& s, float& v);
	static Color hsv_to_color(float h, float s, float v);

	color_util() = delete;
};

bool color_util::is_defined(Color color)
{
	return color.r != 0 || color.g != 0 || color.b != 0 || color.a != 0;
}

Color color_util::with_alpha(Color color, Uint8 alpha)
{
	return Color { color.r, color.g, color.b, alpha };
}

Color color_util::add_rgb(Color colorA, Color colorB)
{
	float r = std::clamp(toF(colorA.r) + toF(colorB.r), 0.0f, 255.0f);
	float g = std::clamp(toF(colorA.g) + toF(colorB.g), 0.0f, 255.0f);
	float b = std::clamp(toF(colorA.b) + toF(colorB.b), 0.0f, 255.0f);

	return Color {
		static_cast<uint8_t>(r),
		static_cast<uint8_t>(g),
		static_cast<uint8_t>(b),
		colorA.a
	};
}

Color color_util::add_rgb(Color color, int value)
{
	int r = std::clamp(toI(color.r) + value, 0, 255);
	int g = std::clamp(toI(color.g) + value, 0, 255);
	int b = std::clamp(toI(color.b) + value, 0, 255);

	return Color {
		static_cast<uint8_t>(r),
		static_cast<uint8_t>(g),
		static_cast<uint8_t>(b),
		color.a
	};
}

Color color_util::add_rgb(Color color, float value)
{
	float r = std::clamp(toF(color.r) + value * 255.0f, 0.0f, 255.0f);
	float g = std::clamp(toF(color.g) + value * 255.0f, 0.0f, 255.0f);
	float b = std::clamp(toF(color.b) + value * 255.0f, 0.0f, 255.0f);

	return Color {
		static_cast<uint8_t>(r),
		static_cast<uint8_t>(g),
		static_cast<uint8_t>(b),
		color.a
	};
}

Color color_util::multiply_rgb(Color colorA, Color colorB)
{
	return Color {
		static_cast<uint8_t>(toF(colorA.r) * toF(colorB.r) / 255.0f),
		static_cast<uint8_t>(toF(colorA.g) * toF(colorB.g) / 255.0f),
		static_cast<uint8_t>(toF(colorA.b) * toF(colorB.b) / 255.0f),
		colorA.a
	};
}

Color color_util::multiply_rgb(Color color, float value)
{
	float r = std::clamp(toF(color.r) * value, 0.0f, 255.0f);
	float g = std::clamp(toF(color.g) * value, 0.0f, 255.0f);
	float b = std::clamp(toF(color.b) * value, 0.0f, 255.0f);

	return Color {
		static_cast<uint8_t>(r),
		static_cast<uint8_t>(g),
		static_cast<uint8_t>(b),
		color.a
	};
}

Colorf color_util::to_colorf(Color color)
{
	return Colorf {
		color.r / 255.0f,
		color.g / 255.0f,
		color.b / 255.0f,
		color.a / 255.0f,
	};
}

Color color_util::to_color(Colorf color)
{
	return Color {
		std::clamp(static_cast<uint8_t>(color.r * 255.0f), 0_u8, 255_u8),
		std::clamp(static_cast<uint8_t>(color.g * 255.0f), 0_u8, 255_u8),
		std::clamp(static_cast<uint8_t>(color.b * 255.0f), 0_u8, 255_u8),
		std::clamp(static_cast<uint8_t>(color.a * 255.0f), 0_u8, 255_u8),
	};
}

Color color_util::color_from_string(std::string hex)
{
	hex = string_util::trim(hex);
	if (hex.empty())
		return (Color)0;
	if (hex[0] == '#')
		hex = hex.erase(0, 1);
	if (hex.length() != 6 && hex.length() != 8)
		return (Color)0;

	try
	{
		if (hex.length() == 6)
		{
			uint8_t r = static_cast<uint8_t>(std::stoi(hex.substr(0, 2), nullptr, 16));
			uint8_t g = static_cast<uint8_t>(std::stoi(hex.substr(2, 2), nullptr, 16));
			uint8_t b = static_cast<uint8_t>(std::stoi(hex.substr(4, 2), nullptr, 16));
			return Color { r, g, b, 0xff };
		}
		else if (hex.length() == 8)
		{
			uint8_t a = static_cast<uint8_t>(std::stoi(hex.substr(0, 2), nullptr, 16));
			uint8_t r = static_cast<uint8_t>(std::stoi(hex.substr(2, 2), nullptr, 16));
			uint8_t g = static_cast<uint8_t>(std::stoi(hex.substr(4, 2), nullptr, 16));
			uint8_t b = static_cast<uint8_t>(std::stoi(hex.substr(6, 2), nullptr, 16));
			return Color { r, g, b, a };
		}
	}
	catch (...)
	{
		return (Color)0;
	}
	return (Color)0;
}

static void RGBtoHSV(uint8_t src_r, uint8_t src_g, uint8_t src_b, float& dst_h, float& dst_s, float& dst_v)
{
	float r = src_r / 255.0f;
	float g = src_g / 255.0f;
	float b = src_b / 255.0f;

	float h, s, v; // h:0-360.0, s:0.0-1.0, v:0.0-1.0

	float min = std::min(r, std::min(g, b));
	float max = std::max(r, std::max(g, b));

	v = max;

	if (max == 0.0f)
	{
		s = 0;
		h = 0;
	}
	else if (max - min == 0.0f)
	{
		s = 0;
		h = 0;
	}
	else
	{
		s = (max - min) / max;

		if (max == r)
		{
			h = 60 * ((g - b) / (max - min)) + 0;
		}
		else if (max == g)
		{
			h = 60 * ((b - r) / (max - min)) + 120;
		}
		else
		{
			h = 60 * ((r - g) / (max - min)) + 240;
		}
	}

	if (h < 0)
		h += 360.0f;

	dst_h = h;
	dst_s = s;
	dst_v = v;
}

static void HSVtoRGB(float h, float s, float v, uint8_t& dst_r, uint8_t& dst_g, uint8_t& dst_b)
{
	float r, g, b; // 0.0-1.0

	int   hi = (int)(h / 60.0f) % 6;
	float f = (h / 60.0f) - hi;
	float p = v * (1.0f - s);
	float q = v * (1.0f - s * f);
	float t = v * (1.0f - s * (1.0f - f));

	switch (hi)
	{
	case 0: r = v, g = t, b = p; break;
	case 1: r = q, g = v, b = p; break;
	case 2: r = p, g = v, b = t; break;
	case 3: r = p, g = q, b = v; break;
	case 4: r = t, g = p, b = v; break;
	case 5: r = v, g = p, b = q; break;
	}

	dst_r = (uint8_t)(r * 255);
	dst_g = (uint8_t)(g * 255);
	dst_b = (uint8_t)(b * 255);
}

void color_util::color_to_hsv(Color color, float& h, float& s, float& v)
{
	RGBtoHSV(color.r, color.g, color.b, h, s, v);
}

Color color_util::hsv_to_color(float h, float s, float v)
{
	uint8_t r, g, b;
	HSVtoRGB(h, s, v, r, g, b);
	return Color { r, g, b, 0xff };
}