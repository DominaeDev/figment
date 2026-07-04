// 
// MIT License
// 
// Helper class for bitwise flag-like operations on scoped enums.
//
// This class provides a way to represent combinations of enum values without
// directly overloading operators on the enum type itself. This approach
// avoids ambiguity in the type system and allows the enum type to continue
// representing a single value, while the EnumFlags can hold a combination
// of enum values.
//
// Example usage:
//
// enum class MyEnum { FlagA = 1 << 0, FlagB = 1 << 1, FlagC = 1 << 2 };
//
// EnumFlags<MyEnum> flags = { MyEnum::FlagA, MyEnum::FlagC };
// flags.Unset(MyEnum::FlagA);
// if (flags.IsSet(MyEnum::FlagC)) {
//   // ...
// }
//
// flags |= MyEnum::FlagB;
// EnumFlags<MyEnum> new_flags = ~flags;
//
// https://voithos.io/articles/type-safe-enum-class-bit-flags/
//

#pragma once

#include <bitset>
#include <type_traits>
#include <utility>
#include <ostream>

namespace fig
{
	template <typename T>
	class EnumFlags {
		static_assert(std::is_enum_v<T>);
	public:
		using UnderlyingType = std::underlying_type_t<T>;

		constexpr EnumFlags() : _flags(static_cast<std::underlying_type_t<T>>(0)) {}
		constexpr explicit EnumFlags(T v) : _flags(ToUnderlying(v)) {}
		constexpr EnumFlags(std::initializer_list<T> vs) : EnumFlags()
		{
			for (T v : vs)
				_flags |= ToUnderlying(v);
		}

		// Checks if a specific flag is set.
		constexpr bool IsSet(T v) const noexcept
		{
			return (_flags & ToUnderlying(v)) == ToUnderlying(v);
		}
		// Checks if a set of flags are set.
		constexpr bool IsSet(std::initializer_list<T> vs) const noexcept
		{
			auto v = EnumFlags(vs);
			return (_flags & v.ToRaw()) == v.ToRaw();
		}
		// Checks if any in a set of flags are set.
		constexpr bool IsAnySet(std::initializer_list<T> vs) const noexcept
		{
			auto v = EnumFlags(vs);
			return (_flags & v.ToRaw()) != static_cast<std::underlying_type_t<T>>(0);
		}
		// Checks if no flag is set.
		constexpr bool IsEmpty() const noexcept { return _flags == static_cast<std::underlying_type_t<T>>(0); }

		// Sets a single flag value.
		constexpr void Set(T v) noexcept { _flags |= ToUnderlying(v); }
		// Unsets a single flag value.
		constexpr void Unset(T v) noexcept { _flags &= ~ToUnderlying(v); }
		// Toggle a single flag value.
		constexpr void Flip(T v) noexcept { IsSet(v) ? Unset(v) : Set(v); }

		// Unsets multiple flag values.
		constexpr void Unset(std::initializer_list<T> vs) noexcept
		{
			auto v = EnumFlags(vs);
			_flags &= ~v.ToRaw();
		}

		// Clears all flag values.
		constexpr void Clear() noexcept { _flags = static_cast<std::underlying_type_t<T>>(0); }

		constexpr operator bool() const noexcept
		{
			return _flags != static_cast<std::underlying_type_t<T>>(0);
		}

		friend constexpr EnumFlags operator|(EnumFlags lhs, T rhs)
		{
			return EnumFlags(lhs._flags | ToUnderlying(rhs));
		}
		friend constexpr EnumFlags operator|(EnumFlags lhs, EnumFlags rhs)
		{
			return EnumFlags(lhs._flags | rhs._flags);
		}
		friend constexpr EnumFlags operator&(EnumFlags lhs, T rhs)
		{
			return EnumFlags(lhs._flags & ToUnderlying(rhs));
		}
		friend constexpr EnumFlags operator&(EnumFlags lhs, EnumFlags rhs)
		{
			return EnumFlags(lhs._flags & rhs._flags);
		}
		friend constexpr EnumFlags operator^(EnumFlags lhs, T rhs)
		{
			return EnumFlags(lhs._flags ^ ToUnderlying(rhs));
		}
		friend constexpr EnumFlags operator^(EnumFlags lhs, EnumFlags rhs)
		{
			return EnumFlags(lhs._flags ^ rhs._flags);
		}

		EnumFlags& operator=(T rhs)
		{
			_flags = ToUnderlying(rhs);
			return *this;
		}

		friend constexpr EnumFlags& operator|=(EnumFlags& lhs, T rhs)
		{
			lhs._flags |= ToUnderlying(rhs);
			return lhs;
		}
		friend constexpr EnumFlags& operator|=(EnumFlags& lhs, EnumFlags rhs)
		{
			lhs._flags |= rhs._flags;
			return lhs;
		}
		friend constexpr EnumFlags& operator&=(EnumFlags& lhs, T rhs)
		{
			lhs._flags &= ToUnderlying(rhs);
			return lhs;
		}
		friend constexpr EnumFlags& operator&=(EnumFlags& lhs, EnumFlags rhs)
		{
			lhs._flags &= rhs._flags;
			return lhs;
		}
		friend constexpr EnumFlags& operator^=(EnumFlags& lhs, T rhs)
		{
			lhs._flags ^= ToUnderlying(rhs);
			return lhs;
		}
		friend constexpr EnumFlags& operator^=(EnumFlags& lhs, EnumFlags rhs)
		{
			lhs._flags ^= rhs._flags;
			return lhs;
		}

		friend constexpr EnumFlags operator~(const EnumFlags& rhs)
		{
			return EnumFlags(~rhs._flags);
		}

		friend constexpr bool operator==(const EnumFlags& lhs, const EnumFlags& rhs)
		{
			return lhs._flags == rhs._flags;
		}
		friend constexpr bool operator!=(const EnumFlags& lhs, const EnumFlags& rhs)
		{
			return lhs._flags != rhs._flags;
		}

		friend constexpr bool operator==(const EnumFlags& lhs, T rhs)
		{
			return lhs._flags == ToUnderlying(rhs);
		}
		friend constexpr bool operator!=(const EnumFlags& lhs, T rhs)
		{
			return lhs._flags != ToUnderlying(rhs);
		}

		auto operator<=>(const EnumFlags& rhs) const
		{
			return _flags <=> rhs._flags;
		}

		// Stream output operator for debugging.
		friend std::ostream& operator<<(std::ostream& os, const EnumFlags& bf)
		{
			// Write out a bitset representation.
			os << std::bitset<sizeof(UnderlyingType) * 8>(bf._flags);
			return os;
		}

		// Construct EnumFlags from raw values.
		static constexpr EnumFlags FromRaw(UnderlyingType flags)
		{
			return EnumFlags(flags);
		}
		// Retrieve the raw underlying flags.
		constexpr UnderlyingType ToRaw() const { return _flags; }

		static const EnumFlags<T> None;
		static const UnderlyingType Zero;
		static const UnderlyingType One;

		template<std::ranges::range Map>
		std::vector<std::string> Serialized(const Map& mapping) const
		{
			std::vector<std::string> result;
			for (auto& [flag, name] : mapping)
			{
				if (IsSet(flag))
					result.push_back(std::string { name });
			}
			return result;
		}

		template<std::ranges::range Map>
		void Deserialized(const std::vector<std::string>& flags, const Map& mapping)
		{
			Clear();
			for (auto& [flag, name] : mapping)
			{
				if (std::ranges::contains(flags, name))
					Set(flag);
			}
		}

		template<std::ranges::range Map>
		static std::vector<std::string> Serialize(EnumFlags flags, const Map& mapping)
		{
			return flags.Serialized(mapping);
		}

		template<std::ranges::range Map>
		static EnumFlags Deserialize(const std::vector<std::string>& values, const Map& mapping)
		{
			EnumFlags flags;
			flags.Deserialized(values, mapping);
			return flags;
		}

	private:
		constexpr explicit EnumFlags(UnderlyingType flags) : _flags(flags) {}
		static constexpr UnderlyingType ToUnderlying(T v) { return static_cast<UnderlyingType>(v); }
		UnderlyingType _flags;
	};

	template <typename T>
	const EnumFlags<T> EnumFlags<T>::None {};

	template <typename T>
	const std::underlying_type_t<T> EnumFlags<T>::Zero { static_cast<std::underlying_type_t<T>>(0) };
}
