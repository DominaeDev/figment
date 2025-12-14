#ifndef TYPES_H__
#define TYPES_H__

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <ranges>
#include <algorithm>
#include "util/EnumFlags.h"

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

typedef std::string string;

inline constexpr string toS(std::string_view sv) { return string(sv); }
inline constexpr const char* toCStr(std::string_view sv) { return static_cast<const char*>(sv.data()); }

// Macros
#if !defined(TRUE)
#define TRUE 1
#endif
#if !defined(FALSE)
#define FALSE 0
#endif

constexpr bool Enabled = true;
constexpr bool Disabled = false;

#endif