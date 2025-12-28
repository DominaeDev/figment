#ifndef CONTEXT_CURSOR_H__
#define CONTEXT_CURSOR_H__
#pragma once

#include <stdint.h>
#include <format>

namespace fig::llm
{
	struct ContextCursor
	{
		explicit ContextCursor(int32_t value = 0);
		ContextCursor(const ContextCursor& other) = default;
		~ContextCursor() = default;

		inline auto operator<=>(const ContextCursor& rhs) const noexcept
		{
			return _value <=> rhs._value;
		}

		inline auto operator<=>(const int32_t& rhs) const noexcept
		{
			return _value <=> rhs;
		}

		inline auto operator==(const int32_t& rhs) const noexcept
		{
			return _value == rhs;
		}

		inline ContextCursor& operator= (int32_t value) noexcept
		{
			_value = value;
			return *this;
		}

		inline ContextCursor& operator= (size_t value) noexcept
		{
			_value = static_cast<int32_t>(value);
			return *this;
		}

		ContextCursor& operator= (const ContextCursor& value) = default;

		ContextCursor operator+ (const ContextCursor& rhs) const
		{
			return ContextCursor(_value + rhs._value);
		}

		ContextCursor operator+ (int32_t rhs) const
		{
			return ContextCursor(_value + rhs);
		}

		ContextCursor operator+ (size_t rhs) const
		{
			return ContextCursor(_value + static_cast<int32_t>(rhs));
		}

		explicit operator int32_t() { return _value; };
		explicit operator size_t() { return static_cast<size_t>(_value); };

		inline ContextCursor& increment(int32_t offset) noexcept
		{
			_value += offset;
			return *this;
		}

		inline ContextCursor& increment(size_t offset) noexcept
		{
			_value += static_cast<int32_t>(offset);
			return *this;
		}

		inline int32_t as_int() const noexcept
		{
			return _value;
		}

		inline int32_t is_valid() const noexcept
		{
			return _value >= 0;
		}

		static ContextCursor Invalid;

	private:
		int32_t _value = 0;
	};
}

template <>
struct std::formatter<fig::llm::ContextCursor> : std::formatter<std::string>
{
	auto format(fig::llm::ContextCursor c, format_context& ctx) const
	{
		return std::formatter<string>::format(std::format("{}", c.as_int()), ctx);
	}
};
#endif