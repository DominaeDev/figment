#pragma once

#include <stdint.h>

namespace fig::io
{
	enum class FileError : uint32_t
	{
		NoError = 0,
		UnknownError,
		NotFound,
		AccessDenied,
		DirectoryDoesNotExist,
		UnrecognizedFormat,
		ReadError,
		WriteError,
		ChecksumError,
	};

	enum class AsyncLoadError : uint32_t
	{
		NoError = 0,
		LoadError,
		FileNotFound,
		FileAccessError,
		Canceled,
	};

	enum class DatabaseError : uint32_t
	{
		NoError = 0,
		NotConnected,
		ZeroChanges,
		FailedContraint,
		SQLError,
	};
}

namespace fig
{
	inline constexpr bool Success(fig::io::FileError error) { return error == fig::io::FileError::NoError; };
	inline constexpr bool Success(fig::io::AsyncLoadError error) { return error == fig::io::AsyncLoadError::NoError; };
	inline constexpr bool Success(fig::io::DatabaseError error) { return error == fig::io::DatabaseError::NoError || error == fig::io::DatabaseError::ZeroChanges; };
	inline constexpr bool Success(bool result) { return result; };
}