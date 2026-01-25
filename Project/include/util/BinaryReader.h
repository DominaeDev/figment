#ifndef BINARY_WRITER_H__
#define BINARY_WRITER_H__
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
		explicit BinaryReader(const fig::string& profileName, fig::security::AESKey key) noexcept;
		std::expected<AssetFile, FileError> ReadFile(const fig::string& filename) noexcept;

	private:
		fig::string _profileName {};
		fig::security::AESKey _authKey {};
	};
}
#endif
