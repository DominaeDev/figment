#include "util/FileUtility.h"
#include "util/StringUtility.h"

#include <fstream>
#include <filesystem>

std::expected<fig::string, FileError> file_util::ReadTextFile(const fig::string& filename, bool normalizeNewlines)
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
			return normalizeNewlines ? string_util::normalize_newlines(content) : content;
		return ""; // Empty file
	}
	catch (...)
	{
		return std::unexpected(FileError::ReadError);
	}
}

FileError file_util::ReadTextFile(const fig::string& filename, fig::string& out_content, bool normalizeNewlines)
{
	auto content = ReadTextFile(filename, normalizeNewlines);
	if (content.has_value())
	{
		out_content = content.value();
		return FileError::NoError;
	}
	return content.error();
}

FileError file_util::WriteTextFile(const fig::string& filename, const fig::string& content, bool append)
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

std::expected<std::vector<fig::string>, FileError> file_util::FindFilesInPath(const fig::string& dirPath, const fig::string& extension)
{
	std::vector<fig::string> matchingFiles;
	std::filesystem::path directory(dirPath);

	if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory))
		return std::unexpected(FileError::DirectoryDoesNotExist);

	for (const auto& entry : std::filesystem::directory_iterator(directory))
	{
		if (std::filesystem::is_regular_file(entry) && (extension.empty() || string_util::equals(entry.path().extension().string(), extension, true)))
			matchingFiles.push_back(entry.path().string());
	}

	return matchingFiles;
}

fig::string file_util::GetFilename(const fig::string& str)
{
	size_t pos = str.find_last_of("\\/"); //! hmm...
	if (pos == fig::string::npos)
		return str;
	return str.substr(pos + 1);
}
