#ifndef COMMON_UTILITY_H__
#define COMMON_UTILITY_H__

#include "Types.h"

#include <optional>
#include <vector>
#include <memory>
#include <iterator>
#include <algorithm>
#include <functional>

namespace fig::common_util
{
	// Debugging
	void Log(fig::string message);
	void LogLn(fig::string message = "");

	template<typename... Args>
	void Log(string fmt, Args... args)
	{
		Log(std::format(fmt, args));
	}

	template<typename... Args>
	void LogLn(string fmt, Args... args)
	{
		LogLn(std::format(fmt, args));
	}

	fig::string CreateUUID();

	fig::string Base64Encode(fig::byte_span data) noexcept;
	fig::bytes Base64Decode(fig::string_view) noexcept;

	template<template <typename, typename> class Cont, typename V, typename A = std::allocator<V>>
	void container_prepend(Cont<V, A>& vecA, const Cont<V, A>& vecB) noexcept
	{
		vecA.insert(std::begin(vecA), std::begin(vecB), std::end(vecB));
	}

	template<template <typename, typename> class Cont, typename V, typename A = std::allocator<V>>
	void container_append(Cont<V, A>& vecA, const Cont<V, A>& vecB) noexcept
	{
		vecA.insert(std::end(vecA), std::begin(vecB), std::end(vecB));
	}

	template<template <typename, typename> class Cont, typename V, typename A = std::allocator<V>>
	Cont<V, A> container_concat(const Cont<V, A>& vecA, const Cont<V, A>& vecB) noexcept
	{
		Cont<V, A> result;
		result.reserve(vecA.size() + vecB.size());
		result.insert(std::end(result), std::begin(vecA), std::end(vecA));
		result.insert(std::end(result), std::begin(vecB), std::end(vecB));
		return result;
	}

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

	template <typename T, typename A = std::allocator<T>>
	constexpr std::vector<T, A>::const_iterator flip_iterator(typename const std::vector<T, A>& vec, typename std::vector<T, A>::const_reverse_iterator rit)
	{
		if (rit != vec.crend())
		{
			auto it = vec.cbegin();
			std::advance(it, (ptrdiff_t)std::distance(rit, vec.crend()) - 1);
			return it;
		}
		return vec.cend();
	}

	template <std::floating_point T>
	inline constexpr int32_t ceil_int(T f)
	{
		return static_cast<int32_t>(std::ceilf(f));
	}

	template <std::floating_point T>
	inline constexpr int32_t floor_int(T f)
	{
		return static_cast<int32_t>(std::floorf(f));
	}
}
#endif