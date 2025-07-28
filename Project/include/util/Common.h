#pragma once

#include <optional>
#include <string>
#include <queue>

// Debugging
extern void DebugPrint(std::string message);
extern void DebugPrintLn(std::string message = "");

// File IO
extern std::optional<std::string> ReadTextFile(const std::string& filename, bool normalizeNewlines = true);
extern bool WriteTextFile(const std::string& filename, const std::string& content, bool append = false);

extern std::string CreateUUID();

template<typename T>
void ClearQueue(std::queue<T>& q)
{
	std::queue<T> empty;
	std::swap(q, empty);
}

template<typename T, typename V = T::value_type>
void AppendVector(T& vecA, const T& vecB) noexcept
{
	vecA.insert(std::end(vecA), std::begin(vecB), std::end(vecB));
}

template<typename T, typename V = T::value_type>
void PrependVector(T& vecA, const T& vecB) noexcept
{
	vecA.insert(std::begin(vecA), std::begin(vecB), std::end(vecB));
}

template<typename T, typename V = T::value_type>
T ConcatVector(const T& vecA, const T& vecB) noexcept
{
	T result;
	result.reserve(vecA.size() + vecB.size());
	result.insert(std::end(result), std::begin(vecA), std::end(vecA));
	result.insert(std::end(result), std::begin(vecB), std::end(vecB));
	return result;
}