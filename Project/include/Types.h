#ifndef TYPES_H__
#define TYPES_H__

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <span>

#include "util/EnumFlags.h"

namespace fig
{
    using string = std::string;
    using string_view = std::string_view;
    using c_string = const char*;
    using wc_string = const wchar_t*;
    using wstring = std::wstring;
    using wstring_view = std::wstring_view;
    using const_string = string_view const;
    static constexpr auto npos = string::npos;
    
    using byte = std::byte;
    using byte_span = std::span<byte>;
    using bytes = std::vector<byte>;
    template<size_t N> 
    using buffer = std::array<byte, N>;
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

inline constexpr uint8_t operator "" _u8( unsigned long long arg ) noexcept
{
    return static_cast<uint8_t>( arg );
}

inline constexpr size_t operator "" _uz( unsigned long long arg ) noexcept
{
    return static_cast<size_t>( arg );
}

inline constexpr fig::string toStr(fig::string_view sv) { return fig::string(sv); }
inline constexpr fig::c_string toCStr(fig::string_view sv) { return static_cast<fig::c_string>(sv.data()); }

constexpr bool Enabled = true;
constexpr bool Disabled = false;

#endif