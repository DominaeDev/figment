#ifndef BINARY_WRITER_H__
#define BINARY_WRITER_H__
#pragma once

#include "util/Serialization.h"
#include "util/Security.h"
#include <fstream>

namespace fig::fs
{
	class BinaryWriter
	{
		BinaryWriter() = delete;
	public:
		explicit BinaryWriter(const fig::string& profileName, fig::security::AESKey key) noexcept;
		FileError WriteFile(const AssetFile& file) noexcept;

	private:
		fig::string _profileName {};
		fig::security::AESKey _authKey {};
	};
}
#endif
