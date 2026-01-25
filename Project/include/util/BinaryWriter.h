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
		explicit BinaryWriter(fig::security::AuthKey key) noexcept;
		FileError WriteFile(const fig::path& directory, const AssetFile& file) noexcept;

	private:
		fig::security::AuthKey _authKey {};
	};
}
#endif
