#include <pch.h>
#include "util/BinaryReader.h"
#include <format>

namespace fig::fs
{
	BinaryReader::BinaryReader(const fig::path& profilePath, fig::security::AuthKey key) noexcept :
		_profilePath { profilePath },
		_authKey { key }
	{
	}

	static bool ReadHeader(std::ifstream& fs, size_t file_size, FileHeader& header)
	{
		// Read header
		fs.read((char*)&header, sizeof(FileHeader));

		// Validate header
		bool valid = header.magic[0] == 'F' && header.magic[1] == 'I' && header.magic[2] == 'G' && header.magic[3] == 'M'; // Magic word
		valid &= VersionNumber(header.fmt_version) == VersionNumber(1, 0); // Format version
		valid &= header.data_offset + header.data_length < file_size; // Data offset
		valid &= (file_size - header.data_offset) % 16 == 0; // Data length should match
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
				uint32_t len;
				fs.read(buf, sizeof(uint32_t));
				len = *reinterpret_cast<uint32_t*>(&buf);

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

	static void ReadData(std::ifstream& fs, AssetFile& file, fig::security::AuthKey authKey) noexcept
	{
		size_t length = file.data_length;
		file.data.resize(length);
		fig::security::Decrypt(fs, file.data, authKey);
	}

	std::expected<AssetFile, FileError> BinaryReader::ReadFile(const fig::string& filename, bool read_data) noexcept
	{
		try
		{
			auto const path = _profilePath / filename;

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

			// Read meta
			if (not ReadMeta(fs, header.meta_count, file.meta))
				return std::unexpected(FileError::UnrecognizedFormat);

			if (header.data_offset == 0xFFFF)
				read_data = false; // Reference: No data

			// Read data
			if (read_data)
			{
				fs.seekg(header.data_offset, std::ios::beg);
				ReadData(fs, file, _authKey);
			}
			return file;
		}
		catch (...)
		{
			return std::unexpected(FileError::ReadError);
		}
	}
}