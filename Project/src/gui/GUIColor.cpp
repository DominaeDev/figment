#include <pch.h>
#include "gui/GUIColor.h"

namespace fig::gui
{
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

	Color Color::WithAlpha(uint8_t alpha) const noexcept
	{
		return Color { r, g, b, alpha };
	}

	Color Color::Add(Color other) const noexcept
	{

		return Color {
			static_cast<uint8_t>(std::clamp(toF(r) + toF(other.r), 0.0f, 255.0f)),
			static_cast<uint8_t>(std::clamp(toF(g) + toF(other.g), 0.0f, 255.0f)),
			static_cast<uint8_t>(std::clamp(toF(b) + toF(other.b), 0.0f, 255.0f)),
			a
		};
	}

	Color Color::Add(int32_t value) const noexcept
	{
		return Color {
			static_cast<uint8_t>(std::clamp(r + value, 0, 255)),
			static_cast<uint8_t>(std::clamp(g + value, 0, 255)),
			static_cast<uint8_t>(std::clamp(b + value, 0, 255)),
			a
		};
	}

	Color Color::Multiply(Color other) const noexcept
	{
		return Color {
			static_cast<uint8_t>(toF(r) * toF(other.r) / 255.0f),
			static_cast<uint8_t>(toF(g) * toF(other.g) / 255.0f),
			static_cast<uint8_t>(toF(b) * toF(other.b) / 255.0f),
			static_cast<uint8_t>(toF(a) * toF(other.a) / 255.0f)
		};
	}

	fig::string Color::ToString() const noexcept
	{
		if (a == 0xFF)
			return std::format("#{:02X}{:02X}{:02X}", r, g, b);
		else
			return std::format("#{:02X}{:02X}{:02X}{:02X}", r, g, b, a);
	}

	Color Color::FromString(const fig::string& value) noexcept
	{
		fig::string hex = fig::util::trim(value);
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
				uint8_t r = static_cast<uint8_t>(std::stoi(hex.substr(0, 2), nullptr, 16));
				uint8_t g = static_cast<uint8_t>(std::stoi(hex.substr(2, 2), nullptr, 16));
				uint8_t b = static_cast<uint8_t>(std::stoi(hex.substr(4, 2), nullptr, 16));
				uint8_t a = static_cast<uint8_t>(std::stoi(hex.substr(6, 2), nullptr, 16));
				return Color { r, g, b, a };
			}
		}
		catch (...)
		{
			return (Color)0;
		}
		return (Color)0;
	}

	std::tuple<float, float, float> Color::GetHSV() const noexcept
	{
		float h, s, v;
		RGBtoHSV(r, g, b, h, s, v);
		return std::make_tuple(h, s, v);
	}

	Color Color::FromHSV(float h, float s, float v) noexcept
	{
		uint8_t r, g, b;
		HSVtoRGB(h, s, v, r, g, b);
		return Color { r, g, b, 0xff };
	}
}