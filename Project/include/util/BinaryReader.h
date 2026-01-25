#ifndef BINARY_READER_H__
#define BINARY_READER_H__
#pragma once

#include "util/Serialization.h"
#include "util/Security.h"
#include <fstream>
#include <expected>

namespace fig::fs
{
	class BinaryReader
	{
		BinaryReader() = delete;
	public:
		explicit BinaryReader(const fig::path& directory, fig::security::AuthKey key) noexcept;
		std::expected<AssetFile, FileError> ReadFile(const fig::string& filename, bool read_data = true) noexcept;

	private:
		fig::path _profilePath {};
		fig::security::AuthKey _authKey {};
	};
}
#endif
