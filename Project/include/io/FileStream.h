#pragma once

#include "Figment.h"
#include "io/Error.h"

#if !USE_WIN32_API
#include <fstream>
#endif

namespace fig::io
{
	struct FileStream
	{
		enum class Flag
		{
			Sequential = 1 << 0,
		};
		using Flags = EnumFlags<Flag>;

#if USE_WIN32_API
		using Handle = ::HANDLE;
#else
		using Handle = std::ifstream&;
#endif

		explicit FileStream(const fig::path& path, Flags flags = {});
		~FileStream();
		FileStream(const FileStream&) = delete;
		FileStream& operator=(const FileStream&) = delete;

		inline bool IsOk() const noexcept { return _bOpen and _error == FileError::NoError; }
		inline FileError GetError() const noexcept { return _error; }

		bool Seek(size_t offset) noexcept;
		size_t Read(fig::bytes& buffer) noexcept;
		size_t Read(fig::bytes& buffer, size_t nBytes) noexcept;
		size_t Read(std::vector<char>& buffer) noexcept;
		size_t Read(std::vector<char>& buffer, size_t nBytes) noexcept;
		template<size_t N>
		size_t Read(fig::buffer<N>& buffer) noexcept
		{
			return Read(reinterpret_cast<char*>(buffer.data()), buffer.size());
		}

		template <typename T>
		bool ReadStruct(T& dst) noexcept {
			return Read(reinterpret_cast<char*>(&dst), sizeof(T)) == sizeof(T);
		}

		inline size_t Length() const noexcept { return _length; }

	private:
		size_t Read(char* pBuf, size_t nBytes) noexcept;

	private:
		fig::path _path {};
		size_t _length {};
		bool _bOpen {};
		FileError _error {};

#if USE_WIN32_API
		::HANDLE _handle = INVALID_HANDLE_VALUE;
		::OVERLAPPED _overlapped {0};
#else
		std::ifstream _fs;
#endif
	};
}
