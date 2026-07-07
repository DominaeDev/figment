#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <span>
#include <map>
#include <stdint.h>
#include <filesystem>
#include <uuid_v4.h>
#include <fixed.hpp>
#include <math.hpp>
#include "util/Handle.h"
#include "util/OptionalRef.h"
#include "util/ExpectedRef.h"
#include "util/ObserverPtr.h"
#include "util/FixedString.h"

namespace fig
{
    // Type aliases

    using string = std::string;
    using string_view = std::string_view;
    using c_string = const char*;
    using wc_string = const wchar_t*;
    using wstring = std::wstring;
    using wstring_view = std::wstring_view;
    using const_string = string_view const;
    using string_cref = const string&;
    using handle = basic_handle<char, 64>;
    using whandle = basic_handle<wchar_t, 0>;

    static constexpr size_t npos = static_cast<size_t>(-1);

    using byte = std::byte;
    using byte_span = std::span<const byte>;
    using bytes = std::vector<byte>;
    template<size_t N> 
    using buffer = std::array<byte, N>;

    using uuid = UUIDv4::UUID;
    using timestamp = uint64_t; // unix epoch (milliseconds)
    using path = std::filesystem::path;
    using string_list = std::vector<string>;
    using string_span = std::span<const string>;
    using fixed = fpm::fixed<std::int32_t, std::int64_t, 10>;

    template<typename T>
    using ref_vector = std::vector<std::reference_wrapper<T>>;
    template<typename T>
    using cref_vector = std::vector<std::reference_wrapper<const T>>;

    // Concepts

    template <typename T>
    concept is_string_like = std::constructible_from<fig::string, T>;

    template <typename T>
    concept is_string_convertible =
        std::constructible_from<T, fig::string> and std::constructible_from<fig::string, T>;

    template<typename T>
    concept is_number = (std::integral<T> || std::floating_point<T>)
        && !std::same_as<T, bool>
        && !std::same_as<T, char>
        && !std::same_as<T, wchar_t>
        && !std::same_as<T, char16_t>
        && !std::same_as<T, char32_t>;

    template<typename T>
    concept is_number_range = std::ranges::range<T>
        && is_number<std::ranges::range_value_t<T>>;

    template<typename T>
    concept is_string_range = std::ranges::range<T>
        && (std::same_as<std::ranges::range_value_t<T>, fig::string> || is_string_like<std::ranges::range_value_t<T>>);
}

// Type conversion functions

template<typename T>
inline constexpr int32_t toI(T x) { return static_cast<int32_t>(x); }
template<typename T>
inline constexpr int64_t toI64(T x) { return static_cast<int64_t>(x); }
template<typename T>
inline constexpr float toF(T x) { return static_cast<float>(x); }
template<typename T>
inline constexpr double toD(T x) { return static_cast<double>(x); }
template<typename T>
inline constexpr size_t toUZ(T x) { return static_cast<size_t>(x); }
template<typename T>
inline constexpr size_t castEnum(T x) { return static_cast<size_t>(x); }

template <typename T>
inline constexpr fig::fixed toFixed(T x)
{
	if constexpr (static_cast<double>(std::numeric_limits<T>::max()) > static_cast<double>(std::numeric_limits<fig::fixed>::max()))
	{
		constexpr T min = static_cast<T>(std::numeric_limits<fig::fixed>::min());
		constexpr T max = static_cast<T>(std::numeric_limits<fig::fixed>::max());
		return static_cast<fig::fixed>(std::clamp(x, min, max));
	}
    else
    {
        return static_cast<fig::fixed>(x);
    }
}

inline constexpr uint8_t operator "" _u8( unsigned long long arg ) noexcept
{
    return static_cast<uint8_t>(arg);
}

inline constexpr size_t operator "" _uz( unsigned long long arg ) noexcept
{
    return static_cast<size_t>(arg);
}

inline constexpr std::byte operator "" _byte( unsigned long long arg ) noexcept
{
    return static_cast<std::byte>(arg);
}

inline constexpr fig::fixed operator "" _fp(unsigned long long arg ) noexcept
{
    return fig::fixed { arg };
}

inline constexpr fig::fixed operator "" _fp( long double arg ) noexcept
{
    return fig::fixed { arg };
}

inline constexpr fig::string toStr(fig::string_view sv) { return fig::string(sv); }
inline constexpr fig::c_string toCStr(fig::string_view sv) { return static_cast<fig::c_string>(sv.data()); }

inline constexpr std::span<const uint8_t> bytes_to_u8(const fig::byte_span& bytes)
{
    return std::span { reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size() };
}

inline constexpr std::span<uint8_t> bytes_to_u8(fig::bytes& bytes)
{
    return std::span { reinterpret_cast<uint8_t*>(bytes.data()), bytes.size() };
}

inline constexpr fig::byte_span u8_to_bytes(const std::span<const uint8_t>& chars)
{
    return std::span { reinterpret_cast<const std::byte*>(chars.data()), chars.size() };
}

inline constexpr fig::byte_span u8_to_bytes(std::span<uint8_t>& chars)
{
    return std::span { reinterpret_cast<const std::byte*>(chars.data()), chars.size() };
}

inline constexpr fig::byte_span string_to_bytes(const fig::string& str)
{
    return std::span { reinterpret_cast<const std::byte*>(str.data()), str.size() };
}

inline constexpr fig::byte_span string_to_bytes(const fig::string_view& sv)
{
    return std::span { reinterpret_cast<const std::byte*>(sv.data()), sv.size() };
}

constexpr bool Enabled = true;
constexpr bool Disabled = false;

#if defined(_DEBUG) || !defined(NDEBUG)
constexpr bool Debugging = true;
#else
constexpr bool Debugging = false;
#endif
