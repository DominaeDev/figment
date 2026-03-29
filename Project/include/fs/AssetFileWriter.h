#ifndef BINARY_WRITER_H__
#define BINARY_WRITER_H__
#pragma once

#include "Types.h"
#include "Constants.h"
#include "util/Security.h"
#include "fs/Serialization.h"

namespace fig::io
{
	class AssetFileWriter
	{
		AssetFileWriter() = delete;
	public:
		explicit AssetFileWriter(const fig::path& directory) noexcept;
		explicit AssetFileWriter(const fig::path& directory, fig::auth::AuthKey key) noexcept;
		FileError WriteFile(const fig::io::AssetFile& file) noexcept;

		static FileError WriteRecoveryFile(const fig::user::UserProfile& profile, const fig::auth::AuthChallenge& recoveryChallenge) noexcept;
		static FileError WriteProfileFile(const fig::user::UserProfile& profile, const fig::path& filename, const fig::io::AssetFile& assetFile) noexcept;

	private:
		fig::path _directory {};
		fig::auth::AuthKey _authKey {};
	};
}
#endif
