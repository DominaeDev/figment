#include <pch.h>
#include "fs/FileStream.h"

namespace fig::io
{
#if USE_WIN32_API
	FileStream::FileStream(const fig::path& path, Flags flags) :
		_path { path }
	{
		DWORD dwFlags = FILE_ATTRIBUTE_NORMAL;
		if (flags.IsSet(Flag::Sequential))
			dwFlags |= FILE_FLAG_SEQUENTIAL_SCAN;
		_fs = ::CreateFileW(path.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, dwFlags, nullptr);

		LARGE_INTEGER liSize {};
		if (::GetFileSizeEx(_fs, &liSize))
			_length = static_cast<size_t>(liSize.QuadPart);

		if (_fs != INVALID_HANDLE_VALUE)
		{
			_error = FileError::NoError;
			_bOpen = true;
		}
		else
		{
			switch (::GetLastError())
			{
			case ERROR_FILE_NOT_FOUND:
			case ERROR_PATH_NOT_FOUND:
				_error = FileError::NotFound;
				break;
			case ERROR_ACCESS_DENIED:
				_error = FileError::AccessDenied;
				break;
			default:
				_error = FileError::UnknownError;
				break;
			}
		}
	}

	FileStream::~FileStream()
	{
		if (_fs != INVALID_HANDLE_VALUE)
			::CloseHandle(_fs);
	}

	bool FileStream::Seek(size_t offset) noexcept
	{
		if (!IsOk())
			return false;

		LARGE_INTEGER li;
		li.QuadPart = (LONGLONG)offset;
		return ::SetFilePointerEx(_fs, li, nullptr, FILE_BEGIN) != FALSE;
	}

	size_t FileStream::Read(char* pBuf, size_t nBytes) noexcept
	{
		if (!IsOk())
			return 0uz;

		size_t total_read = 0uz;
		while (nBytes > 0)
		{
			DWORD read = 0;
			if (!::ReadFile(_fs, (LPVOID)pBuf, (DWORD)nBytes, &read, nullptr) || read == 0)
				break;

			std::advance(pBuf, read);
			nBytes -= read;
			total_read += read;
		}
		return total_read;
	}
#else
	FileStream::FileStream(const fig::path& path, Flags flags) :
		_path { path }
	{
		_fs = std::ifstream { path.wstring(), std::ios::binary | std::ios::in | std::ios::ate};
		if (_fs.is_open())
		{
			_length = static_cast<size_t>(_fs.tellg());
			_fs.seekg(0, std::ios::beg);
			_bOpen = true;
		}
		else
		{
			int err = errno;
			switch (err)
			{
			case ENOENT:
				_error = FileError::NotFound;
			case EACCES:
				_error = FileError::AccessDenied;
			default:
				_error = FileError::UnknownError;
			}
		}
	}

	FileStream::~FileStream()
	{
	}

	bool FileStream::Seek(size_t offset) noexcept
	{
		if (IsOk())
		{
			_fs.seekg(offset, std::ios::beg);
			return true;
		}
		return false;
	}

	size_t FileStream::Read(char* pBuf, size_t nBytes) noexcept
	{
		if (IsOk())
		{
			_fs.read(pBuf, nBytes);
			return static_cast<size_t>(_fs.gcount());
		}
		return 0uz;
	}
#endif

	size_t FileStream::Read(fig::bytes& buffer) noexcept
	{
		return Read((char*)(buffer.data()), buffer.size());
	}

	size_t FileStream::Read(fig::bytes& buffer, size_t nBytes) noexcept
	{
		return Read((char*)(buffer.data()), std::min(nBytes, buffer.size()));
	}

	size_t FileStream::Read(std::vector<char>& buffer) noexcept
	{
		return Read(reinterpret_cast<char*>(buffer.data()), buffer.size());
	}

	size_t FileStream::Read(std::vector<char>& buffer, size_t nBytes) noexcept
	{
		return Read(reinterpret_cast<char*>(buffer.data()), std::min(nBytes, buffer.size()));
	}
}