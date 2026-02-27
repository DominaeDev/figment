#pragma once

#include <stdint.h>

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
};
