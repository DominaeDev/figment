#pragma once

#include <optional>
#include <string>
#include <queue>
#include <memory>


// Debugging
extern void DebugPrint(std::string message);
extern void DebugPrintLn(std::string message = "");

// File IO
extern std::optional<std::string> ReadTextFile(const std::string& filename, bool normalizeNewlines = true);
extern bool WriteTextFile(const std::string& filename, const std::string& content, bool append = false);
extern std::vector<std::string> FindFilesInPath(const std::string& dirPath, const std::string& extension);

extern std::string CreateUUID();

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

template <std::ranges::range R>
constexpr auto to_vector(R&& r)
{
	using elem_t = std::decay_t<std::ranges::range_value_t<R>>;
	return std::vector<elem_t>{r.begin(), r.end()};
}