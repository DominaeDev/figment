#include <pch.h>
#include "fs/BinaryReader.h"
#include "model/UserProfile.h"
#include <Crc32.h>
#include <format>

using namespace fig::io::data;

namespace fig::io
{
	BinaryReader::BinaryReader(const fig::path& directory, fig::user::auth::AuthKey key) noexcept :
		_directory { directory },
		_authKey { key }
	{
	}

	static bool ReadHeader(std::ifstream& fs, size_t file_size, FileHeader& header)
	{
		// Read header
		fs.read((char*)&header, sizeof(FileHeader));

		// Validate header
		bool valid = header.magic[0] == MagicWord[0] && header.magic[1] == MagicWord[1] && header.magic[2] == MagicWord[2] && header.magic[3] == MagicWord[3];
		
		// Version
		valid &= header.header_version == 1;

		// Data extents
		if (valid and header.data_length != 0)
		{
			size_t data_offset = header.data_offset + sizeof(FileHeader);
			valid &= (data_offset + header.data_length <= file_size);
			valid &= (!(bool)(header.flags & FileHeaderFlag::Encrypted)) or ((file_size - data_offset) % 16 == 0); // Encrypted data length must be divisible by 16
		}
		return valid;
	}

	static bool ReadMeta(std::ifstream& fs, uint8_t count, std::map<MetaTag, MetaValue>& outMeta)
	{
		char buf[16] = {};
		for (size_t i = 0; i < toUZ(count); ++i)
		{
			MetaTag tag;
			fs.read((char*)(&tag), sizeof(MetaTag));
			if (fs.eof() or fs.bad())
				return false; // Error

			MetaValueType type = get_meta_type(tag);
			if (type == MetaValueType::Unknown)
				return false; // Unrecognized tag

			switch (type)
			{
			case MetaValueType::Boolean:
				fs.read(buf, sizeof(uint8_t));
				outMeta[tag] = (bool)(*reinterpret_cast<uint8_t*>(&buf));
				break;
			case MetaValueType::UChar:
				fs.read(buf, sizeof(uint8_t));
				outMeta[tag] = *reinterpret_cast<uint8_t*>(&buf);
				break;
			case MetaValueType::UShort:
				fs.read(buf, sizeof(uint16_t));
				outMeta[tag] = *reinterpret_cast<uint16_t*>(&buf);
				break;
			case MetaValueType::Integer:
				fs.read(buf, sizeof(int32_t));
				outMeta[tag] = *reinterpret_cast<int32_t*>(&buf);
				break;
			case MetaValueType::Float:
				fs.read(buf, sizeof(float));
				outMeta[tag] = *reinterpret_cast<float*>(&buf);
				break;
			case MetaValueType::TimeStamp:
				fs.read(buf, sizeof(fig::timestamp));
				outMeta[tag] = *reinterpret_cast<fig::timestamp*>(&buf);
				break;
			case MetaValueType::Identifier:
				fs.read(buf, sizeof(_meta_identifier));
				outMeta[tag] = *reinterpret_cast<_meta_identifier*>(&buf);
				break;
			case MetaValueType::String:
			{
				// Read length
				uint8_t len;
				fs.read(buf, sizeof(uint8_t));
				len = *reinterpret_cast<uint8_t*>(&buf);

				// Read data
				std::vector<char> strbuf(len);
				fs.read(strbuf.data(), len);
				outMeta[tag] = fig::string(strbuf.cbegin(), strbuf.cend());
				break;
			}
			default:
				return false; // Error?
			};
		}
		return true;
	}

	static void ReadData(std::ifstream& fs, AssetFile& file, fig::user::auth::AuthKey authKey) noexcept
	{
		size_t length = file.data_length;
		file.data.resize(length);
		if (file.data_encrypted and !fig::util::is_zero(authKey))
		{
			// Read encrypted
			fig::user::auth::Decrypt(fs, file.data, authKey);
		}
		else
		{
			// Read unencrypted
			fs.read((char*)file.data.data(), file.data.size());
		}
	}

	static std::expected<AssetFile, FileError> __ReadFile(const fig::path& path, bool read_data, fig::user::auth::AuthKey authKey) noexcept
	{
		try
		{
			std::ifstream fs(path.wstring(), std::ios::binary | std::ios::in | std::ios::ate);
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

			// Get file size
			std::streamsize file_size = fs.tellg();
			fs.seekg(0, std::ios::beg);
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
				fs.seekg(header.data_offset + sizeof(FileHeader), std::ios::beg);
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