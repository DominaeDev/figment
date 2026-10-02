#pragma once

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

	FileError DeleteFile(const fig::path& path) noexcept;
		
	template <std::ranges::range T>
	requires std::is_same_v<fig::path, std::ranges::range_value_t<T>>
	size_t DeleteFiles(const T& paths) noexcept
	{
		size_t count = 0uz;
		for (auto& path : paths)
		{
			if (DeleteFile(path) == FileError::NoError)
				count++;
		}
		return count;
	}
	
	fig::path GetUserDataFolder();
	fig::path GetUserDataFilename(const fig::path& filename);
	fig::path GetTemporaryFolder();
	fig::path GetTemporaryFilename(const fig::path& filename);
	fig::path GetPackagesFolder();
	fig::path GetPackagesFilename(const fig::path& filename);
	fig::path GetProfilesFolder();
	fig::path GetProfilesFilename(const fig::path& path);
	bool EnsureFolderExists(fig::path path);

	std::expected<std::vector<fig::path>, FileError> FindFilesInPath(const fig::path& directory, const fig::string& extension);

	// Utility
	fig::string GetFilename(const fig::string& str);
	fig::string GetFileExt(fig::path filename);

	std::expected<fig::string, FileError> ReadPNGMeta(fig::path filename, const fig::string& keyword = "chara", bool bDecodeBase64 = true) noexcept;
	std::expected<fig::string, FileError> ReadPNGMeta(const fig::bytes& buffer, const fig::string& keyword = "chara", bool bDecodeBase64 = true) noexcept;

}
