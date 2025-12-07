module;

#include <stdio.h>
#include <print>
#include <ranges>
#include <uuid_v4.h>
#include <queue>

export module Utility:Common;

import :StringUtil;

using string = std::string;

export 
{
	// Debugging
	void DebugPrint(string message)
	{
#if _DEBUG
		if (message.empty())
			return;

		std::print(stdout, "{}", message);
#else
		// noop
#endif
	}

	void DebugPrintLn(string message = "")
	{
#if _DEBUG
		if (message.empty())
			return;

		std::println(stdout, "{}", message);
#else
		// noop
#endif
	}

	string CreateUUID()
	{
		static UUIDv4::UUIDGenerator<std::mt19937_64> uuidGenerator;
		return uuidGenerator.getUUID().str();
	}

	template<typename T>
	void ClearQueue(std::queue<T>& q)
	{
		std::queue<T> empty;
		std::swap(q, empty);
	}

	template<template <typename, typename> class Cont, typename V, typename A = std::allocator<V>>
	void ContainerAppend(Cont<V, A>& vecA, const Cont<V, A>& vecB) noexcept
	{
		vecA.insert(std::end(vecA), std::begin(vecB), std::end(vecB));
	}

	template<template <typename, typename> class Cont, typename V, typename A = std::allocator<V>>
	void ContainerPrepend(Cont<V, A>& vecA, const Cont<V, A>& vecB) noexcept
	{
		vecA.insert(std::begin(vecA), std::begin(vecB), std::end(vecB));
	}

	template<template <typename, typename> class Cont, typename V, typename A = std::allocator<V>>
	Cont<V, A> ContainerConcat(const Cont<V, A>& vecA, const Cont<V, A>& vecB) noexcept
	{
		Cont<V, A> result;
		result.reserve(vecA.size() + vecB.size());
		result.insert(std::end(result), std::begin(vecA), std::end(vecA));
		result.insert(std::end(result), std::begin(vecB), std::end(vecB));
		return result;
	}

	template<template <typename, typename> class Cont, typename V, typename A = std::allocator<V>, typename Predicate>
	Cont<V, A> FilterContainer(const Cont<V, A>& input, Predicate pred)
	{
		Cont<V, A> result;
		std::copy_if(input.begin(), input.end(), std::back_inserter(result), pred);
		return result;
	}

	template <std::ranges::range R>
	constexpr auto to_vector(R&& r)
	{
		using elem_t = std::decay_t<std::ranges::range_value_t<R>>;
		return std::vector<elem_t>{r.begin(), r.end()};
	}
}