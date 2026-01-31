#include <pch.h>
#include "util/FileUtility.h"
#include "util/StringUtility.h"
#include "model/Asset.h"

#include <fstream>

using namespace fig::string_util;

namespace fig::fs
{
	std::expected<fig::bytes, FileError> ReadFile(fig::path filename)
	{
		try
		{
			std::ifstream file(filename.wstring(), std::ios::binary | std::ios::in | std::ios::ate);
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

	FileError WriteFile(fig::path filename, fig::byte_span data)
	{
		try
		{
			std::ofstream file(filename.wstring(), std::ios::binary | std::ios::out | std::ios::trunc);
			if (not file.is_open())
				return FileError::WriteError;

			file.write((const char*)(data.data()), data.size());
			return FileError::NoError;
		}
		catch (...)
		{
			return FileError::WriteError;
		}
	}

	std::expected<fig::string, FileError> ReadTextFile(const fig::string& filename, bool normalizeNewlines)
	{
		try
		{
			std::ifstream file(filename.c_str(), std::ios::binary | std::ios::in | std::ios::ate);
			if (!file)
				return std::unexpected(FileError::FileNotFound);

			std::streamsize size = file.tellg();
			file.seekg(0, std::ios::beg);

			fig::string content;
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

	FileError ReadTextFile(const fig::string& filename, fig::string& out_content, bool normalizeNewlines)
	{
		auto content = ReadTextFile(filename, normalizeNewlines);
		if (content.has_value())
		{
			out_content = content.value();
			return FileError::NoError;
		}
		return content.error();
	}

	FileError WriteTextFile(const fig::string& filename, const fig::string& content, bool append)
	{
		try
		{
			std::ofstream file(filename.c_str(), std::ios::binary | std::ios::out | (append ? std::ios::app : std::ios::trunc));
			if (!file.is_open())
				return FileError::WriteError;

			file.write(content.c_str(), content.length());
			return file.fail() ? FileError::WriteError : FileError::NoError;
		}
		catch (...)
		{
			return FileError::WriteError;
		}
	}

	std::expected<std::vector<fig::string>, FileError> FindFilesInPath(const fig::string& dirPath, const fig::string& extension)
	{
		std::vector<fig::string> matchingFiles;
		fig::path directory(dirPath);

		if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory))
			return std::unexpected(FileError::DirectoryDoesNotExist);

		for (const auto& entry : std::filesystem::directory_iterator(directory))
		{
			if (std::filesystem::is_regular_file(entry) && (extension.empty() || equals(entry.path().extension().u8string(), extension, true)))
				matchingFiles.push_back(entry.path().u8string());
		}

		return matchingFiles;
	}

	fig::string GetFilename(const fig::string& str)
	{
		size_t pos = str.find_last_of("\\/"); //! hmm...
		if (pos == string::npos)
			return str;
		return str.substr(pos + 1);
	}

	fig::string GetFileExt(fig::path filename)
	{
		return string_util::lcase(fig::path(filename).extension().u8string());
	}
}