#pragma once

#include <SDL3/SDL.h>
#include <algorithm>
#include <tuple>

namespace fig
{
	using colorf = SDL_FColor;

	struct color : SDL_Color
	{
		constexpr color() noexcept
		{
			this->r = 0x00;
			this->g = 0x00;
			this->b = 0x00;
			this->a = 0x00;
		}

		constexpr explicit color(int32_t rgb) noexcept : color(rgb, 0xFF)
		{}
		
		constexpr explicit color(int32_t rgb, uint8_t a) noexcept
		{
			this->r = (rgb & 0xFF0000) >> 16;
			this->g = (rgb & 0xFF00) >> 8;
			this->b = (rgb & 0xFF);
			this->a = a;
		}

		constexpr color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xFF) noexcept
		{
			this->r = r;
			this->g = g;
			this->b = b;
			this->a = a;
		}

		constexpr color(const fig::colorf color) noexcept
		{
			this->r = std::clamp(static_cast<uint8_t>(color.r * 255.0f), 0_u8, 255_u8);
			this->g = std::clamp(static_cast<uint8_t>(color.g * 255.0f), 0_u8, 255_u8);
			this->b = std::clamp(static_cast<uint8_t>(color.b * 255.0f), 0_u8, 255_u8);
			this->a = std::clamp(static_cast<uint8_t>(color.a * 255.0f), 0_u8, 255_u8);
		}

		template<std::floating_point T>
		constexpr explicit color(T r, T g, T b, T a = T(1.0)) noexcept
		{
			this->r = static_cast<uint8_t>(std::clamp(r, T(0), T(1)) * T(255));
			this->g = static_cast<uint8_t>(std::clamp(g, T(0), T(1)) * T(255));
			this->b = static_cast<uint8_t>(std::clamp(b, T(0), T(1)) * T(255));
			this->a = static_cast<uint8_t>(std::clamp(a, T(0), T(1)) * T(255));
		}

		inline constexpr bool IsDefined() const noexcept
		{
			return r != 0 || g != 0 || b != 0 || a != 0;
		}

		std::tuple<float, float, float> GetHSV() const noexcept;

		color Add(color other) const noexcept;
		color Add(int32_t value) const noexcept;
		color Multiply(color other) const noexcept;
		color WithAlpha(uint8_t alpha) const noexcept;
		color Blend(color other, float t) const noexcept
		{
			t = std::clamp(t, 0.0f, 1.0f);
			return {
				static_cast<Uint8>(std::lerp(static_cast<float>(r), static_cast<float>(other.r), t)),
				static_cast<Uint8>(std::lerp(static_cast<float>(g), static_cast<float>(other.g), t)),
				static_cast<Uint8>(std::lerp(static_cast<float>(b), static_cast<float>(other.b), t)),
				static_cast<Uint8>(std::lerp(static_cast<float>(a), static_cast<float>(other.a), t))
			};
		}
		
		template<std::floating_point T>
		color Add(T value) const noexcept
		{
			return Add(static_cast<uint8_t>(std::clamp(value, T(0), T(1)) * T(255)));
		}

		template<std::floating_point T>
		color Multiply(T value) const noexcept
		{
			return color {
				static_cast<uint8_t>(std::clamp(T(r) * value, T(0), T(255))),
				static_cast<uint8_t>(std::clamp(T(g) * value, T(0), T(255))),
				static_cast<uint8_t>(std::clamp(T(b) * value, T(0), T(255))),
				a,
			};
		}

		template<std::floating_point T>
		color WithAlpha(T alpha) const noexcept
		{
			return WithAlpha(static_cast<uint8_t>(std::clamp(alpha, T(0), T(1)) * T(255)));
		}

		inline operator colorf() const noexcept
		{
			return fig::colorf { r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f };
		}

		inline explicit operator uint32_t() const noexcept
		{
			return std::bit_cast<uint32_t>(*this);
		}

		fig::string ToString() const noexcept;
		static color FromString(fig::string_view value) noexcept;
		static color FromHSV(float h, float s, float v) noexcept;
	};

	inline constexpr color operator "" _argb(unsigned long long arg) noexcept
	{
		const auto value = static_cast<uint32_t>(arg);
		return color { static_cast<int32_t>(value & 0xFFFFFF), static_cast<uint8_t>(value >> 24) };
	}

	inline constexpr color operator "" _rgba(unsigned long long arg) noexcept
	{
		const auto value = static_cast<uint32_t>(arg);
		return color { static_cast<int32_t>(value >> 8), static_cast<uint8_t>(value & 0xFF) };
	}

	inline constexpr color operator "" _rgb(unsigned long long arg) noexcept
	{
		return color { static_cast<int32_t>(arg & 0xFFFFFF) };
	}
}
