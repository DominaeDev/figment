#ifndef BINARY_WRITER_H__
#define BINARY_WRITER_H__
#pragma once

#include "Types.h"
#include "Constants.h"
#include "util/Security.h"
#include "fs/Serialization.h"

namespace fig::io
{
	class BinaryWriter
	{
		BinaryWriter() = delete;
	public:
		explicit BinaryWriter(const fig::path& directory) noexcept;
		explicit BinaryWriter(const fig::path& directory, fig::user::auth::AuthKey key) noexcept;
		FileError WriteFile(const fig::io::data::AssetFile& file) noexcept;

		static FileError WriteRecoveryFile(const fig::user::UserProfile& profile, const fig::user::auth::AuthChallenge& recoveryChallenge) noexcept;
		static FileError WriteProfileFile(const fig::user::UserProfile& profile, const fig::path& filename, const fig::io::data::AssetFile& assetFile) noexcept;

	private:
		fig::path _directory {};
		fig::user::auth::AuthKey _authKey {};
	};
}
#endif
