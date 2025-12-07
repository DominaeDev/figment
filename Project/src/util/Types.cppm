export module Types;

// Standard library (must come before module imports)
/*#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <format>
#include <cassert>
#include <set>
#include <functional>
#include <ostream>*/

export import <cstdint>;
export import <cwchar>;
export import <cassert>;
export import std;
export import EnumFlags;

export
{
	// User literal
	inline constexpr uint8_t operator "" _u8(unsigned long long arg) noexcept
	{
		return static_cast<uint8_t>(arg);
	}

	using string = std::string;

	template <typename T, typename A = std::allocator<T>>
	constexpr std::vector<T, A>::iterator flip_iterator(typename std::vector<T, A>& vec, typename std::vector<T, A>::reverse_iterator rit)
	{
		if (rit != vec.rend())
		{
			auto it = vec.begin();
			std::advance(it, (ptrdiff_t)std::distance(rit, vec.rend()) - 1);
			return it;
		}
		return vec.end();
	}

	template<typename T>
	inline constexpr int32_t toI(T x) { return static_cast<int32_t>(x); }
	template<typename T>
	inline constexpr int64_t toI64(T x) { return static_cast<int64_t>(x); }
	template<typename T>
	inline constexpr float toF(T x) { return static_cast<float>(x); }
	template<typename T>
	inline constexpr double toD(T x) { return static_cast<double>(x); }
	template<typename T>
	inline constexpr size_t toSZ(T x) { return static_cast<size_t>(x); }
	template<typename T>
	inline constexpr size_t castEnum(T x) { return static_cast<size_t>(x); }

	constexpr bool Enabled = true;
	constexpr bool Disabled = false;
}