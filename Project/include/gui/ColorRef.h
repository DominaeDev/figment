#pragma once

#include "gui/Color.h"

namespace fig::gui
{
	enum class Color;
}

namespace fig
{
	class color_ref_with_alpha;

	class color_ref
	{
		static constexpr const fig::color null_value { 0xCDCDCD00_rgba };
	public:
		constexpr color_ref() noexcept :
			_ptr { &null_value }
		{
		}
		constexpr color_ref(fig::color* color) noexcept :
			_ptr { color }
		{
		}
		constexpr color_ref(const fig::color* color) noexcept :
			_ptr { color }
		{
		}
		constexpr color_ref(color_ref&& other) noexcept :
			_ptr { other._ptr }
		{
		}

		color_ref(fig::gui::Color color);

		constexpr color_ref(const color_ref&) = default;
		color_ref& operator= (const color_ref& other) = default;
		color_ref& operator= (color_ref&& other) noexcept
		{
			_ptr = other._ptr;
			return *this;
		};

		const fig::color& operator*() const noexcept { return *_ptr; }
//		const fig::color* const operator->() const noexcept { return _ptr; }
		const fig::color* const get() const noexcept { return _ptr; }

		constexpr bool IsDefined() const noexcept { return _ptr != &null_value; }
		explicit operator bool() const noexcept { return IsDefined(); }

		inline constexpr operator fig::color() const noexcept { return *_ptr; }
		inline constexpr operator fig::colorf() const noexcept 
		{
			return fig::colorf { 
				static_cast<float>(_ptr->r) / 255.0f, 
				static_cast<float>(_ptr->g) / 255.0f, 
				static_cast<float>(_ptr->b) / 255.0f, 
				static_cast<float>(_ptr->a) / 255.0f 
			};
		}

		color_ref_with_alpha WithAlpha(uint8_t alpha) const noexcept;

		template<std::floating_point T>
		constexpr fig::color_ref_with_alpha WithAlpha(T alpha) const noexcept
		{
			auto alpha_u8 = static_cast<uint8_t>(std::clamp(alpha, T(0), T(1)) * T(255));
			return WithAlpha(alpha_u8);
		}

		inline constexpr uint8_t r() const noexcept { return _ptr->r; }
		inline constexpr uint8_t g() const noexcept { return _ptr->g; }
		inline constexpr uint8_t b() const noexcept { return _ptr->b; }
		inline constexpr uint8_t a() const noexcept { return _ptr->a; }

	protected:
		const fig::color* _ptr;

	public:
		static const color_ref nullref;
	};

	class color_ref_with_alpha : public color_ref
	{
	public:
		constexpr color_ref_with_alpha() noexcept : color_ref(),
			_a { 0 }
		{
		}
		explicit constexpr color_ref_with_alpha(fig::color* color) noexcept : color_ref(color),
			_a { color->a }
		{
		}
		explicit constexpr color_ref_with_alpha(const fig::color* color) noexcept : color_ref(color),
			_a { color->a }
		{
		}
		color_ref_with_alpha(const color_ref& other) : color_ref(other),
			_a { other.a() }
		{
		}
		color_ref_with_alpha(const color_ref& other, uint8_t alpha) : color_ref(other),
			_a { alpha }
		{
		}

		constexpr color_ref_with_alpha(const color_ref_with_alpha&) = default;
		explicit constexpr color_ref_with_alpha(color_ref_with_alpha&& other) noexcept :
			_a { other._a }
		{
			_ptr = other._ptr;
		}

		color_ref_with_alpha(fig::gui::Color color) : color_ref(color)
		{
			_a = _ptr->a;
		}

		color_ref_with_alpha& operator= (const color_ref_with_alpha& other) = default;
		color_ref_with_alpha& operator= (color_ref_with_alpha&& other) noexcept
		{
			_ptr = other._ptr;
			_a = other._a;
			return *this;
		};

		color_ref_with_alpha& operator= (const color_ref& other) noexcept 
		{
			_ptr = other.get();
			_a = _ptr->a;
			return *this;
		};

		color_ref_with_alpha& operator= (color_ref&& other) noexcept
		{
			_ptr = other.get();
			_a = _ptr->a;
			return *this;
		};


		fig::color operator*() const noexcept { return fig::color(_ptr->r, _ptr->g, _ptr->b, _a); }

		color_ref_with_alpha WithAlpha(uint8_t alpha) const noexcept
		{
			auto copy { *this };
			copy._a = alpha;
			return copy;
		}

		template<std::floating_point T>
		constexpr fig::color_ref_with_alpha WithAlpha(T alpha) const noexcept
		{
			return WithAlpha(static_cast<uint8_t>(std::clamp(alpha, T(0), T(1)) * T(255)));
		}

		constexpr void SetAlpha(uint8_t alpha) noexcept
		{
			_a = alpha;
		}

		template<std::floating_point T>
		constexpr void SetAlpha(T alpha) noexcept
		{
			_a = static_cast<uint8_t>(std::clamp(alpha, T(0), T(1)) * T(255));
		}

		inline constexpr operator fig::color() const noexcept
		{
			return fig::color(_ptr->r, _ptr->g, _ptr->b, _a);
		}

		void ResetAlpha(uint8_t alpha) noexcept
		{
			_a = _ptr->a;
		}


		inline constexpr operator fig::colorf() const noexcept
		{
			return fig::colorf {
				static_cast<float>(_ptr->r) / 255.0f,
				static_cast<float>(_ptr->g) / 255.0f,
				static_cast<float>(_ptr->b) / 255.0f,
				static_cast<float>(_a)		/ 255.0f
			};
		}

		inline constexpr uint8_t r() const noexcept { return _ptr->r; }
		inline constexpr uint8_t g() const noexcept { return _ptr->g; }
		inline constexpr uint8_t b() const noexcept { return _ptr->b; }
		inline constexpr uint8_t a() const noexcept { return _a; }

	private:
		uint8_t _a;

	public:
		static const color_ref nullref;
	};
}