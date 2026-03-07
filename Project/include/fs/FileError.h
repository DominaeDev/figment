#pragma once

#include <stdint.h>

namespace fig::io
{
	enum class FileError : uint32_t
	{
		NoError = 0,
		FileNotFound,
		FileAccessError,
		DirectoryDoesNotExist,
		UnrecognizedFormat,
		ReadError,
		WriteError,
		ChecksumError,
		Canceled,
	};
}