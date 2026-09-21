#pragma once

#include "gui/Color.h"

namespace fig::gui
{
	enum class Colour;
}

namespace fig
{
	class color_ref
	{
		static constexpr const fig::color null { 0xCDCDCD00_rgba };
	public:
		constexpr color_ref() noexcept :
			_ptr { &null }
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

		color_ref(fig::gui::Colour color);

		constexpr color_ref(const color_ref&) = default;
		color_ref& operator= (const color_ref& other) = default;
		color_ref& operator= (color_ref&& other)
		{
			_ptr = other._ptr;
		};

		const fig::color& operator*() const noexcept { return *_ptr; }
		const fig::color* const operator->() const noexcept { return _ptr; }

		fig::color_ref WithAlpha(uint8_t alpha) const noexcept;
		
		template<std::floating_point T>
		constexpr fig::color_ref WithAlpha(T alpha) const noexcept
		{
			auto alpha_u8 = static_cast<uint8_t>(std::clamp(alpha, T(0), T(1)) * T(255));
			return WithAlpha(alpha_u8);
		}

		inline constexpr operator fig::color() const noexcept
		{
			return *_ptr;
		}

		inline constexpr operator fig::colorf() const noexcept
		{
			return fig::colorf { _ptr->r / 255.0f, _ptr->g / 255.0f, _ptr->b / 255.0f, _ptr->a / 255.0f };
		}

		inline constexpr uint8_t r() const noexcept { return _ptr->r; }
		inline constexpr uint8_t g() const noexcept { return _ptr->g; }
		inline constexpr uint8_t b() const noexcept { return _ptr->b; }
		inline constexpr uint8_t a() const noexcept { return _ptr->a; }

	private:
		const fig::color* _ptr;

	public:
		static color_ref nullref;
	};
}