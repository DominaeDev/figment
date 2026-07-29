#include <pch.h>
#include "io/AssetFileReader.h"
#include "io/FileStream.h"
#include "user/UserProfile.h"
#include <Crc32.h>

namespace fig::io
{
	constexpr uint8_t FileHeaderVersion = 1;

	AssetFileReader::AssetFileReader(const fig::path& directory, fig::auth::AuthKey key) noexcept :
		_directory { directory },
		_authKey { key }
	{
	}

	static bool ReadMeta(FileStream& fs, size_t data_offset, std::map<MetaTag, MetaValue>& outMeta)
	{
		std::vector<char> buf(16);
		for (; fs.GetPosition() < data_offset;)
		{
			MetaTag tag;
			if (!fs.ReadStruct(tag))
				return false; // Error

			MetaValueType type = get_meta_type(tag);
			if (type == MetaValueType::Unknown)
				return false; // Unrecognized tag

			switch (type)
			{
			case MetaValueType::Boolean:
				fs.Read(buf, sizeof(uint8_t));
				outMeta[tag] = (bool)(*reinterpret_cast<uint8_t*>(buf.data()));
				break;
			case MetaValueType::UChar:
				fs.Read(buf, sizeof(uint8_t));
				outMeta[tag] = *reinterpret_cast<uint8_t*>(buf.data());
				break;
			case MetaValueType::UShort:
				fs.Read(buf, sizeof(uint16_t));
				outMeta[tag] = *reinterpret_cast<uint16_t*>(buf.data());
				break;
			case MetaValueType::Integer:
				fs.Read(buf, sizeof(int32_t));
				outMeta[tag] = *reinterpret_cast<int32_t*>(buf.data());
				break;
			case MetaValueType::Float:
				fs.Read(buf, sizeof(float));
				outMeta[tag] = *reinterpret_cast<float*>(buf.data());
				break;
			case MetaValueType::TimeStamp:
			{
				fs.Read(buf, sizeof(int64_t));
				int64_t valuei64 = *reinterpret_cast<int64_t*>(buf.data());
				outMeta[tag] = fig::timestamp(valuei64, fig::timezone::global);
				break;
			}
			case MetaValueType::Identifier:
				fs.Read(buf, sizeof(_meta_identifier));
				outMeta[tag] = *reinterpret_cast<_meta_identifier*>(buf.data());
				break;
			case MetaValueType::String:
			{
				// Read length
				fs.Read(buf, sizeof(uint8_t));
				uint8_t len = *reinterpret_cast<uint8_t*>(buf.data());

				// Read data
				std::vector<char> strbuf(len);
				fs.Read(strbuf);
				outMeta[tag] = fig::string(strbuf.cbegin(), strbuf.cend());
				break;
			}
			default:
				return false; // Unrecognized tag
			};
		}
		return true;
	}

	static bool ReadHeader(FileStream& fs, size_t file_size, AssetFileHeader& header)
	{
		// Read header
		if (!fs.ReadStruct(header))
			return false;

		// Validate header
		bool valid = 
			   header.magic[0] == AssetFileHeader::MagicWord[0] 
			&& header.magic[1] == AssetFileHeader::MagicWord[1]
			&& header.magic[2] == AssetFileHeader::MagicWord[2] 
			&& header.magic[3] == AssetFileHeader::MagicWord[3];

		// Version
		valid &= header.header_version == FileHeaderVersion; //! @versioning

		// Data offset
		valid &= header.data_offset >= sizeof(AssetFileHeader);

		// Data length
		if (valid and header.data_length != 0)
		{
			size_t data_offset = header.data_offset;
			valid &= (data_offset + header.data_length <= file_size);
			valid &= !header.flags.IsSet(AssetFileHeaderFlag::Encrypted) or ((file_size - data_offset) % 16uz == 0); // Encrypted data length must be a multiple of 16
		}
		return valid;
	}

	static void ReadData(FileStream& fs, AssetFile& file, fig::auth::AuthKey authKey) noexcept
	{
		size_t length = file.data_length;
		file.data.resize(length);
		if (file.data_encrypted and not is_zero(authKey))
		{
			// Read encrypted
			fig::auth::Decrypt(fs, file.data, authKey);
		}
		else
		{
			// Read unencrypted
			fs.Read(file.data, file.data.size());
		}
	}

	[[nodiscard]] static std::expected<AssetFile, FileError> __ReadFile(const fig::path& path, bool read_data, fig::auth::AuthKey authKey) noexcept
	{
		try
		{
			FileStream fs(path, { FileStream::Flag::Sequential });

			if (not fs.IsOk())
				return std::unexpected(fs.GetError());
			
			size_t file_size = fs.Length();
			if (file_size < sizeof(AssetFileHeader))
				return std::unexpected(FileError::UnrecognizedFormat);

			AssetFile file {};
			AssetFileHeader header {};

			if (not ReadHeader(fs, file_size, header))
				return std::unexpected(FileError::UnrecognizedFormat);

			auto parent_id = reinterpret_cast<uint64_t*>(&header.parent_id);
			file.parent_id = fig::uuid(parent_id[1], parent_id[0]);
			auto asset_id = reinterpret_cast<uint64_t*>(&header.asset_id);
			file.asset_id = fig::uuid(asset_id[1], asset_id[0]);
			file.type = header.asset_type;
			file.data_length = header.data_length;
			file.data_encrypted = (bool)(header.flags & AssetFileHeaderFlag::Encrypted);

			// Read meta
			if (not ReadMeta(fs, header.data_offset, file.meta))
				return std::unexpected(FileError::UnrecognizedFormat);

			// Read data
			if (read_data and header.data_length > 0)
			{
				fs.Seek(header.data_offset);
				ReadData(fs, file, authKey);

				int32_t checksum;
				if ((bool)(header.flags & AssetFileHeaderFlag::Checksum) and file.try_get_meta(MetaTag::Checksum, checksum))
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

	std::expected<AssetFile, FileError> AssetFileReader::ReadFile(const fig::path& filename, bool read_data) noexcept
	{
		auto const path = _directory / filename;
		return __ReadFile(path, read_data, _authKey);
	}

	std::expected<fig::io::AssetFile, FileError> AssetFileReader::ReadProfileFile(const fig::user::UserProfile& profile, const fig::path& filename) noexcept
	{
		auto const path = profile.GetPath() / filename;
		return __ReadFile(path, true, {});
	}
}