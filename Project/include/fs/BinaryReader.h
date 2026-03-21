#ifndef BINARY_READER_H__
#define BINARY_READER_H__
#pragma once

#include "Types.h"
#include "Constants.h"
#include "util/Security.h"
#include "fs/Serialization.h"

namespace fig::io
{
	class BinaryReader
	{
		BinaryReader() = delete;
	public:
		explicit BinaryReader(const fig::path& directory, fig::user::auth::AuthKey key) noexcept;
		std::expected<fig::io::data::AssetFile, FileError> ReadFile(const fig::path& filename, bool read_data = true) noexcept;

		static std::expected<fig::io::data::AssetFile, FileError> ReadProfileFile(const fig::user::UserProfile& profile, const fig::path& filename) noexcept;
	private:
		fig::path _directory {};
		fig::user::auth::AuthKey _authKey {};
	};
}

#endif
