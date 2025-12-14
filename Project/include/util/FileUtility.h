#ifndef FILE_UTILITY_H__
#define FILE_UTILITY_H__

#include "Types.h"
#include <expected>

enum class FileError : uint32_t
{
	NoError = 0,
	FileNotFound,
	DirectoryDoesNotExist,
	ReadError,
	WriteError,
};

namespace fig::file_util
{
	// File IO
	std::expected<string, FileError> ReadTextFile(const string& filename, bool normalizeNewlines = true);
	FileError ReadTextFile(const string& filename, string& out_content, bool normalizeNewlines = true);
	FileError WriteTextFile(const string& filename, const string& content, bool append = false);
	std::expected<std::vector<string>, FileError> FindFilesInPath(const string& dirPath, const string& extension);

	// Utility
	string GetFilename(const string& str);
}

#endif