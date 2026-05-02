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
}