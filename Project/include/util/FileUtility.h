#ifndef FILE_UTILITY_H__
#define FILE_UTILITY_H__

#include <expected>
#include <string>
#include <vector>

enum class FileError : uint32_t
{
	NoError = 0,
	FileNotFound,
	DirectoryDoesNotExist,
	ReadError,
	WriteError,
};

namespace file_util
{
	// File IO
	std::expected<std::string, FileError> ReadTextFile(const std::string& filename, bool normalizeNewlines = true);
	FileError ReadTextFile(const std::string& filename, std::string& out_content, bool normalizeNewlines = true);
	FileError WriteTextFile(const std::string& filename, const std::string& content, bool append = false);
	std::expected<std::vector<std::string>, FileError> FindFilesInPath(const std::string& dirPath, const std::string& extension);
}

#endif