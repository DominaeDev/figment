#include <pch.h>
#include "fs/BinaryReader.h"
#include "model/UserProfile.h"
#include <Crc32.h>
#include <format>

using namespace fig::io::data;

#if USE_WIN32_API
using FileHandle = ::HANDLE;
using DataPtr = char*;
#else
using FileHandle = std::ifstream&;
using DataPtr = char*;
#endif

namespace fig::io
{
	constexpr uint8_t FileHeaderVersion = 1;

	BinaryReader::BinaryReader(const fig::path& directory, fig::user::auth::AuthKey key) noexcept :
		_directory { directory },
		_authKey { key }
	{
	}

	static bool _ReadBytes(FileHandle h, DataPtr buf, size_t num_bytes) noexcept;
	static bool _Seek(FileHandle h, size_t offset) noexcept;

	static bool ReadMeta(FileHandle fs, uint8_t count, std::map<MetaTag, MetaValue>& outMeta)
	{
		char buf[16] = {};
		for (size_t i = 0; i < toUZ(count); ++i)
		{
			MetaTag tag;
			if (!_ReadBytes(fs, (char*)(&tag), sizeof(MetaTag)))
				return false; // Error

			MetaValueType type = get_meta_type(tag);
			if (type == MetaValueType::Unknown)
				return false; // Unrecognized tag

			switch (type)
			{
			case MetaValueType::Boolean:
				_ReadBytes(fs, buf, sizeof(uint8_t));
				outMeta[tag] = (bool)(*reinterpret_cast<uint8_t*>(&buf));
				break;
			case MetaValueType::UChar:
				_ReadBytes(fs, buf, sizeof(uint8_t));
				outMeta[tag] = *reinterpret_cast<uint8_t*>(&buf);
				break;
			case MetaValueType::UShort:
				_ReadBytes(fs, buf, sizeof(uint16_t));
				outMeta[tag] = *reinterpret_cast<uint16_t*>(&buf);
				break;
			case MetaValueType::Integer:
				_ReadBytes(fs, buf, sizeof(int32_t));
				outMeta[tag] = *reinterpret_cast<int32_t*>(&buf);
				break;
			case MetaValueType::Float:
				_ReadBytes(fs, buf, sizeof(float));
				outMeta[tag] = *reinterpret_cast<float*>(&buf);
				break;
			case MetaValueType::TimeStamp:
				_ReadBytes(fs, buf, sizeof(fig::timestamp));
				outMeta[tag] = *reinterpret_cast<fig::timestamp*>(&buf);
				break;
			case MetaValueType::Identifier:
				_ReadBytes(fs, buf, sizeof(_meta_identifier));
				outMeta[tag] = *reinterpret_cast<_meta_identifier*>(&buf);
				break;
			case MetaValueType::String:
			{
				// Read length
				_ReadBytes(fs, buf, sizeof(uint8_t));
				uint8_t len;
				len = *reinterpret_cast<uint8_t*>(&buf);

				// Read data
				std::vector<char> strbuf(len);
				_ReadBytes(fs, strbuf.data(), len);
				outMeta[tag] = fig::string(strbuf.cbegin(), strbuf.cend());
				break;
			}
			default:
				return false; // Unrecognized tag
			};
		}
		return true;
	}

	static bool ReadHeader(FileHandle fs, size_t file_size, FileHeader& header)
	{
		// Read header
		if (!_ReadBytes(fs, (char*)&header, sizeof(FileHeader)))
			return false;

		// Validate header
		bool valid = header.magic[0] == MagicWord[0] && header.magic[1] == MagicWord[1] && header.magic[2] == MagicWord[2] && header.magic[3] == MagicWord[3];

		// Version
		valid &= header.header_version == FileHeaderVersion;

		// Data extents
		if (valid and header.data_length != 0)
		{
			size_t data_offset = header.data_offset + sizeof(FileHeader);
			valid &= (data_offset + header.data_length <= file_size);
			valid &= (!(bool)(header.flags & FileHeaderFlag::Encrypted)) or ((file_size - data_offset) % 16 == 0); // Encrypted data length must be divisible by 16
		}
		return valid;
	}

	static void ReadData(FileHandle fs, AssetFile& file, fig::user::auth::AuthKey authKey) noexcept
	{
		size_t length = file.data_length;
		file.data.resize(length);
		if (file.data_encrypted and not fig::util::is_zero(authKey))
		{
			// Read encrypted
			fig::user::auth::Decrypt(fs, file.data, authKey);
		}
		else
		{
			// Read unencrypted
			_ReadBytes(fs, (DataPtr)file.data.data(), file.data.size());
		}
	}

#if USE_WIN32_API
	struct _File
	{
		HANDLE _handle = INVALID_HANDLE_VALUE;

		explicit _File(HANDLE handle) : _handle(handle) {}
		~_File()
		{
			if (_handle != INVALID_HANDLE_VALUE)
				::CloseHandle(_handle);
		}

		_File(const _File&) = delete;
		_File& operator=(const _File&) = delete;

		bool ok()  const { return _handle != INVALID_HANDLE_VALUE; }
		operator FileHandle() const { return _handle; }
	};

	static bool _ReadBytes(FileHandle h, DataPtr buf, size_t num_bytes) noexcept
	{
		size_t remaining = num_bytes;
		while (remaining > 0)
		{
			DWORD read = 0;
			if (!::ReadFile(h, (LPVOID)buf, (DWORD)remaining, &read, nullptr) || read == 0)
				break;
			buf += ptrdiff_t(read);
			remaining -= read;
		}
		return remaining == 0;
	}

	static bool _Seek(FileHandle h, size_t offset) noexcept
	{
		LARGE_INTEGER li;
		li.QuadPart = (LONGLONG)offset;
		return ::SetFilePointerEx(h, li, nullptr, FILE_BEGIN) != FALSE;
	}
#else
	static bool _ReadBytes(FileHandle fs, DataPtr buf, size_t num_bytes) noexcept
	{
		if (buf == nullptr)
			return false;

		fs.read((char*)buf, num_bytes);
		if (static_cast<size_t>(fs.gcount()) < num_bytes)
			return false;
		return not (fs.eof() or fs.bad());
	}

	static bool _Seek(FileHandle h, size_t offset) noexcept
	{
		h.seekg(offset, std::ios::beg);
		return true;
	}
#endif

	[[nodiscard]] static std::expected<AssetFile, FileError> __ReadFile(const fig::path& path, bool read_data, fig::user::auth::AuthKey authKey) noexcept
	{
		try
		{

#if USE_WIN32_API
			_File fs { ::CreateFileW(path.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr) };

			if (!fs.ok())
			{
				switch (::GetLastError())
				{
				case ERROR_FILE_NOT_FOUND:
				case ERROR_PATH_NOT_FOUND:
					return std::unexpected(FileError::FileNotFound);
				case ERROR_ACCESS_DENIED:
					return std::unexpected(FileError::FileAccessError);
				default:
					return std::unexpected(FileError::ReadError);
				}
			}
#else
			std::ifstream fs(path.wstring(), std::ios::binary | std::ios::in);
			if (fs.fail())
			{
				int err = errno;
				switch (err)
				{
				case ENOENT:
					return std::unexpected(FileError::FileNotFound);
				case EACCES:
					return std::unexpected(FileError::FileAccessError);
				default:
					return std::unexpected(FileError::ReadError);
				}
			}

			if (not fs.is_open())
				return std::unexpected(FileError::FileNotFound);
#endif

			// Get file size
			size_t file_size = std::filesystem::file_size(path);
			if (file_size < sizeof(FileHeader))
				return std::unexpected(FileError::UnrecognizedFormat);

			AssetFile file {};
			FileHeader header {};

			if (not ReadHeader(fs, file_size, header))
				return std::unexpected(FileError::UnrecognizedFormat);

			auto parent_id = reinterpret_cast<uint64_t*>(&header.parent_id);
			file.parent_id = fig::uuid(parent_id[1], parent_id[0]);
			auto asset_id = reinterpret_cast<uint64_t*>(&header.asset_id);
			file.asset_id = fig::uuid(asset_id[1], asset_id[0]);
			file.asset_type = header.asset_type;
			file.asset_subtype = header.asset_subtype;
			file.data_format = header.data_format;
			file.data_length = header.data_length;
			file.data_encrypted = (bool)(header.flags & FileHeaderFlag::Encrypted);

			// Read meta
			if (not ReadMeta(fs, header.meta_count, file.meta))
				return std::unexpected(FileError::UnrecognizedFormat);

			// Read data
			if (read_data and header.data_length > 0)
			{
				_Seek(fs, header.data_offset + sizeof(FileHeader));
				ReadData(fs, file, authKey);

				int32_t checksum;
				if ((bool)(header.flags & FileHeaderFlag::Checksum) and file.try_get_meta(MetaTag::Checksum, checksum))
				{
					// Compare checksum
					int32_t crc32 = static_cast<int32_t>(crc32_fast(file.data.data(), file.data.size()));
					if (crc32 != checksum)
						return std::unexpected(FileError::ChecksumError);
				}
			}
			return file;
		}
		catch (...)
		{
			return std::unexpected(FileError::ReadError);
		}
	}

	std::expected<AssetFile, FileError> BinaryReader::ReadFile(const fig::path& filename, bool read_data) noexcept
	{
		auto const path = _directory / filename;
		return __ReadFile(path, read_data, _authKey);
	}

	std::expected<fig::io::data::AssetFile, FileError> BinaryReader::ReadProfileFile(const fig::user::UserProfile& profile, const fig::path& filename) noexcept
	{
		auto const path = profile.GetPath() / filename;
		return __ReadFile(path, true, {});
	}
}