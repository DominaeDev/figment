#include <pch.h>
#include "util/FileUtility.h"
#include "util/StringUtility.h"

#include <fstream>
#include <filesystem>

using namespace fig::string_util;

namespace fig::fs
{
	std::expected<fig::bytes, FileError> ReadFile(const fig::string& filename)
	{
		try
		{
			auto const path = std::filesystem::path(filename.c_str());
			std::ifstream file(path.wstring(), std::ios::binary | std::ios::in | std::ios::ate);
			if (!file)
				return std::unexpected(FileError::FileNotFound);

			std::streamsize size = file.tellg();
			file.seekg(0, std::ios::beg);
			if (size == 0uz)
				return std::unexpected(FileError::ReadError); // Empty file

			fig::bytes content(size);
			file.read(reinterpret_cast<char*>(content.data()), size);
			return content; // rvo
		}
		catch (...)
		{
			return std::unexpected(FileError::ReadError);
		}
	}

	std::expected<string, FileError> ReadTextFile(const string& filename, bool normalizeNewlines)
	{
		try
		{
			std::ifstream file(filename.c_str(), std::ios::binary | std::ios::in | std::ios::ate);
			if (!file)
				return std::unexpected(FileError::FileNotFound);

			std::streamsize size = file.tellg();
			file.seekg(0, std::ios::beg);

			string content;
			content.reserve(size);
			content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
			if (!content.empty())
				return normalizeNewlines ? normalize_newlines(content) : content;
			return ""; // Empty file
		}
		catch (...)
		{
			return std::unexpected(FileError::ReadError);
		}
	}

	FileError ReadTextFile(const string& filename, string& out_content, bool normalizeNewlines)
	{
		auto content = ReadTextFile(filename, normalizeNewlines);
		if (content.has_value())
		{
			out_content = content.value();
			return FileError::NoError;
		}
		return content.error();
	}

	FileError WriteTextFile(const string& filename, const string& content, bool append)
	{
		try
		{
			std::ofstream file(filename.c_str(), std::ios::binary | std::ios::out | (append ? std::ios::app : std::ios::trunc));
			if (!file.is_open())
				return FileError::FileNotFound;

			file.write(content.c_str(), content.length());
			return file.fail() ? FileError::WriteError : FileError::NoError;
		}
		catch (...)
		{
			return FileError::WriteError;
		}
	}

	std::expected<std::vector<string>, FileError> FindFilesInPath(const string& dirPath, const string& extension)
	{
		std::vector<string> matchingFiles;
		std::filesystem::path directory(dirPath);

		if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory))
			return std::unexpected(FileError::DirectoryDoesNotExist);

		for (const auto& entry : std::filesystem::directory_iterator(directory))
		{
			if (std::filesystem::is_regular_file(entry) && (extension.empty() || equals(entry.path().extension().string(), extension, true)))
				matchingFiles.push_back(entry.path().string());
		}

		return matchingFiles;
	}

	string GetFilename(const string& str)
	{
		size_t pos = str.find_last_of("\\/"); //! hmm...
		if (pos == string::npos)
			return str;
		return str.substr(pos + 1);
	}
}