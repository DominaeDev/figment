#ifndef FILE_UTILITY_H__
#define FILE_UTILITY_H__

#include "Figment.h"
#include <expected>
#include "io/Error.h"

namespace fig::io
{
	// File IO
	std::expected<fig::bytes, FileError> ReadFile(fig::path filename);
	FileError WriteFile(fig::path filename, fig::byte_span data);

	std::expected<fig::string, FileError> ReadTextFile(const fig::path& filename, bool normalizeNewlines = true);
	FileError ReadTextFile(const fig::path& filename, fig::string& out_content, bool normalizeNewlines = true);
	FileError WriteTextFile(const fig::path& filename, const fig::string& content, bool append = false);

	std::expected<std::vector<fig::path>, FileError> FindFilesInPath(const fig::path& directory, const fig::string& extension);

	// Utility
	fig::string GetFilename(const fig::string& str);
	fig::string GetFileExt(fig::path filename);

	std::expected<fig::string, FileError> ReadPNGMeta(fig::path filename, const fig::string& keyword = "chara", bool bDecodeBase64 = true) noexcept;
	std::expected<fig::string, FileError> ReadPNGMeta(const fig::bytes& buffer, const fig::string& keyword = "chara", bool bDecodeBase64 = true) noexcept;
}

#endif