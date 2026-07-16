#include <pch.h>
#include "io/FileStream.h"

namespace fig::io
{
#if USE_WIN32_API
	FileStream::FileStream(const fig::path& path, Flags flags) :
		_path { path }
	{
		DWORD dwFlags = FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_NORMAL;
		if (flags.IsSet(Flag::Sequential))
			dwFlags |= FILE_FLAG_SEQUENTIAL_SCAN;
		_handle = ::CreateFileW(path.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, dwFlags, nullptr);

		if (_handle != INVALID_HANDLE_VALUE)
		{
			_overlapped.Offset = 0;
			_overlapped.OffsetHigh = 0;
			_overlapped.hEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);

			LARGE_INTEGER liSize {};
			if (::GetFileSizeEx(_handle, &liSize))
				_length = static_cast<size_t>(liSize.QuadPart);

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
		if (_handle != INVALID_HANDLE_VALUE)
		{
			::CloseHandle(_overlapped.hEvent);
			::CloseHandle(_handle);
		}
	}

	bool FileStream::Seek(size_t offset) noexcept
	{
		if (!IsOk())
			return false;

		ULARGE_INTEGER li;
		li.QuadPart = (ULONGLONG)offset;
		_overlapped.Offset = li.LowPart;
		_overlapped.OffsetHigh = li.HighPart;
		return true;
	}

	size_t FileStream::Read(char* pBuf, size_t nBytes) noexcept
	{
		if (!IsOk())
			return 0uz;

		size_t total_read = 0uz;
		while (nBytes > 0)
		{
			BOOL result = ::ReadFile(_handle, (LPVOID)pBuf, (DWORD)nBytes, nullptr, &_overlapped);
			if (!result && ::GetLastError() != ERROR_IO_PENDING)
				break;
			::WaitForSingleObject(_overlapped.hEvent, INFINITE);
			DWORD read = 0;
			if (!::GetOverlappedResult(_handle, &_overlapped, &read, FALSE) || read == 0)
				break;

			// Advance the file offset manually
			ULARGE_INTEGER offset;
			offset.LowPart = _overlapped.Offset;
			offset.HighPart = _overlapped.OffsetHigh;
			offset.QuadPart += read;
			_overlapped.Offset = offset.LowPart;
			_overlapped.OffsetHigh = offset.HighPart;

			std::advance(pBuf, read);
			nBytes -= read;
			total_read += read;
		}
		return total_read;
	}
#else // !USE_WIN32_API

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
				break;
			case EACCES:
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