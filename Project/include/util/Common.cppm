module;

#include <ranges>
#include <string>
#include <queue>
#include <optional>
#include <vector>
#include "util/StringUtility.h"

export module Utility:Common;

import std;
export import "uuid_v4.h";

export 
{
	// Debugging
	void DebugPrint(std::string message)
	{
#if _DEBUG
		if (message.empty())
			return;

		printf(message.c_str());
#else
		// noop
#endif
	}

	void DebugPrintLn(std::string message = "")
	{
		DebugPrint(message);
		DebugPrint("\r\n");
	}

	// File IO
	std::optional<std::string> ReadTextFile(const std::string& filename, bool normalizeNewlines = true)
	{
		try
		{
			std::ifstream file(filename.c_str(), std::ios::binary | std::ios::in | std::ios::ate);
			if (!file)
				return std::nullopt;

			std::streamsize size = file.tellg();
			file.seekg(0, std::ios::beg);

			std::string content;
			content.reserve(size);
			content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
			if (!content.empty())
				return normalizeNewlines ? string_util::normalize_newlines(content) : content;
			return std::nullopt; // Empty file
		}
		catch (...)
		{
			return std::nullopt;
		}
	}

	bool ReadTextFile(const std::string& filename, std::string& out_content, bool normalizeNewlines)
	{
		auto content = ReadTextFile(filename, normalizeNewlines);
		if (content.has_value())
		{
			out_content = content.value();
			return true;
		}
		return false;
	}

	bool WriteTextFile(const std::string& filename, const std::string& content, bool append = false)
	{
		try
		{
			std::ofstream file(filename.c_str(), std::ios::binary | std::ios::out | (append ? std::ios::app : std::ios::trunc));
			if (!file.is_open())
				return false;

			file.write(content.c_str(), content.length());
			return !file.fail();
		}
		catch (...)
		{
			return false;
		}
	}

	std::string CreateUUID()
	{
		static UUIDv4::UUIDGenerator<std::mt19937_64> uuidGenerator;
		return uuidGenerator.getUUID().str();
	}

	std::vector<std::string> FindFilesInPath(const std::string& dirPath, const std::string& extension)
	{
		std::vector<std::string> matchingFiles;
		std::filesystem::path directory(dirPath);

		if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory))
			return matchingFiles;

		for (const auto& entry : std::filesystem::directory_iterator(directory))
		{
			if (std::filesystem::is_regular_file(entry) && (extension.empty() || string_util::equals(entry.path().extension().string(), extension, true)))
				matchingFiles.push_back(entry.path().string());
		}

		return matchingFiles;
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