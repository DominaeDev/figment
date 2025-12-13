#ifndef COMMON_UTILITY_H__
#define COMMON_UTILITY_H__

#include <optional>
#include <string>
#include <queue>
#include <memory>
#include <iterator>
#include <algorithm>

namespace common_util
{
	// Debugging
	void DebugPrint(std::string message) noexcept;
	void DebugPrintLn(std::string message = "") noexcept;

	std::string CreateUUID();

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
}
#endif