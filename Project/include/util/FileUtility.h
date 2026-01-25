#ifndef FILE_UTILITY_H__
#define FILE_UTILITY_H__

#include "Types.h"
#include <expected>
#include "util/Serialization.h"

namespace fig::fs
{
	// File IO
	std::expected<fig::bytes, FileError> ReadFile(const string& filename);
	FileError WriteFile(const fig::string& filename, fig::byte_span data);

	std::expected<string, FileError> ReadTextFile(const string& filename, bool normalizeNewlines = true);
	FileError ReadTextFile(const string& filename, string& out_content, bool normalizeNewlines = true);
	FileError WriteTextFile(const string& filename, const string& content, bool append = false);
	std::expected<std::vector<string>, FileError> FindFilesInPath(const string& dirPath, const string& extension);

	// Utility
	string GetFilename(const string& str);
}

#endif