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

namespace file_util
{
	// File IO
	std::expected<fig::string, FileError> ReadTextFile(const fig::string& filename, bool normalizeNewlines = true);
	FileError ReadTextFile(const fig::string& filename, fig::string& out_content, bool normalizeNewlines = true);
	FileError WriteTextFile(const fig::string& filename, const fig::string& content, bool append = false);
	std::expected<std::vector<fig::string>, FileError> FindFilesInPath(const fig::string& dirPath, const fig::string& extension);

	// Utility
	fig::string GetFilename(const fig::string& str);

}

#endif