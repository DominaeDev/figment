#ifndef COMMON_UTILITY_H__
#define COMMON_UTILITY_H__

#include "Types.h"
#include <optional>
#include <vector>
#include <queue>
#include <memory>
#include <iterator>

namespace common_util
{
	// Debugging
	void DebugPrint(fig::string message) noexcept;
	void DebugPrintLn(fig::string message = "") noexcept;

	fig::string CreateUUID();

	template<typename T>
	void queue_clear(std::queue<T>& q)
	{
		std::queue<T> empty;
		std::swap(q, empty);
	}

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
}
#endif