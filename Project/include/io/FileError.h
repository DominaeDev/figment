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

	inline constexpr bool Success(FileError error) { return error == FileError::NoError; };
	inline constexpr bool Success(AsyncLoadError error) { return error == AsyncLoadError::NoError; };
	inline constexpr bool Success(bool result) { return result; };
}