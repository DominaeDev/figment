#ifndef BINARY_READER_H__
#define BINARY_READER_H__
#pragma once

#include "Figment.h"
#include "user/Security.h"
#include "io/Serialization.h"

namespace fig::io
{
	class AssetFileReader
	{
		AssetFileReader() = delete;
	public:
		explicit AssetFileReader(const fig::path& directory, fig::auth::AuthKey key) noexcept;
		std::expected<fig::io::AssetFile, FileError> ReadFile(const fig::path& filename, bool read_data = true) noexcept;

		static std::expected<fig::io::AssetFile, FileError> ReadProfileFile(const fig::user::UserProfile& profile, const fig::path& filename) noexcept;
	private:
		fig::path _directory {};
		fig::auth::AuthKey _authKey {};
	};
}

#endif
