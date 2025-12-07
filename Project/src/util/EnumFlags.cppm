// https://voithos.io/articles/type-safe-enum-class-bit-flags/
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

export module EnumFlags;

import std;

export
{

	template <typename T>
	class EnumFlags {
		static_assert(std::is_enum_v<T>);
	public:
		using UnderlyingType = std::underlying_type_t<T>;

		constexpr EnumFlags() : _flags(Zero) {}
		constexpr explicit EnumFlags(T v) : _flags(ToUnderlying(v)) {}
		constexpr EnumFlags(std::initializer_list<T> vs) : EnumFlags()
		{
			for (T v : vs)
				_flags |= ToUnderlying(v);
		}

		// Checks if a specific flag is set.
		constexpr bool IsSet(T v) const
		{
			return (_flags & ToUnderlying(v)) == ToUnderlying(v);
		}
		// Checks if a set of flags are set.
		constexpr bool IsSet(std::initializer_list<T> vs) const
		{
			auto v = EnumFlags(vs);
			return (_flags & v.ToRaw()) == v.ToRaw();
		}
		// Checks if any in a set of flags are set.
		constexpr bool IsAnySet(std::initializer_list<T> vs) const
		{
			auto v = EnumFlags(vs);
			return (_flags & v.ToRaw()) != Zero;
		}
		// Checks if no flag is set.
		constexpr bool IsEmpty() const { return _flags == Zero; }

		// Sets a single flag value.
		constexpr void Set(T v) { _flags |= ToUnderlying(v); }
		// Unsets a single flag value.
		constexpr void Unset(T v) { _flags &= ~ToUnderlying(v); }
		// Clears all flag values.
		constexpr void Clear() { _flags = Zero; }

		constexpr operator bool() const
		{
			return _flags != Zero;
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

		friend constexpr EnumFlags operator~(const EnumFlags& bf)
		{
			return EnumFlags(~bf._flags);
		}

		friend constexpr bool operator==(const EnumFlags& lhs, const EnumFlags& rhs)
		{
			return lhs._flags == rhs._flags;
		}
		friend constexpr bool operator!=(const EnumFlags& lhs, const EnumFlags& rhs)
		{
			return lhs._flags != rhs._flags;
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

	private:
		constexpr explicit EnumFlags(UnderlyingType flags) : _flags(flags) {}
		static constexpr UnderlyingType ToUnderlying(T v) { return static_cast<UnderlyingType>(v); }
		UnderlyingType _flags;
	};

	template <typename T>
	const EnumFlags<T> EnumFlags<T>::None {};

	template <typename T>
	const EnumFlags<T>::UnderlyingType EnumFlags<T>::Zero = static_cast<EnumFlags<T>::UnderlyingType>(0);
}