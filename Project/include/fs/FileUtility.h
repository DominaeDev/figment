#ifndef FILE_UTILITY_H__
#define FILE_UTILITY_H__

#include "Types.h"
#include <expected>
#include "fs/Serialization.h"

namespace fig::fs
{
	// File IO
	std::expected<fig::bytes, FileError> ReadFile(fig::path filename);
	FileError WriteFile(fig::path filename, fig::byte_span data);

	std::expected<fig::string, FileError> ReadTextFile(const fig::string& filename, bool normalizeNewlines = true);
	FileError ReadTextFile(const fig::string& filename, fig::string& out_content, bool normalizeNewlines = true);
	FileError WriteTextFile(const fig::string& filename, const fig::string& content, bool append = false);

	std::expected<std::vector<fig::string>, FileError> FindFilesInPath(const fig::string& dirPath, const fig::string& extension);

	// Utility
	fig::string GetFilename(const fig::string& str);
	fig::string GetFileExt(fig::path filename);
}

#endif