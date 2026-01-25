#include <pch.h>
#include "util/BinaryReader.h"
#include <format>
#include <filesystem>

namespace fig::fs
{
	BinaryReader::BinaryReader(const fig::string& profileName, fig::security::AESKey key) noexcept :
		_profileName { profileName },
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

	static void ReadData(std::ifstream& fs, AssetFile& file, fig::security::AESKey authKey) noexcept
	{
		size_t length = file.data_length;
		file.data.resize(length);
		fig::security::Decrypt(fs, file.data, authKey);
	}

	std::expected<AssetFile, FileError> BinaryReader::ReadFile(const fig::string& filename) noexcept
	{
		try
		{
			auto const path = std::filesystem::path(std::format("{0}/{1}/{2}", Constants::Paths::ProfilesFolder, _profileName, filename));

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

			auto parent_id = reinterpret_cast<uint32_t*>(&header.parent_id);
			file.parent_id = fig::uuid(parent_id[0], parent_id[1]);
			auto asset_id = reinterpret_cast<uint32_t*>(&header.asset_id);
			file.asset_id = fig::uuid(asset_id[0], asset_id[1]);
			file.asset_type = header.asset_type;
			file.asset_subtype = header.asset_subtype;
			file.data_format = header.data_format;
			file.data_length = header.data_length;

			// Read meta
			if (not ReadMeta(fs, header.meta_count, file.meta))
				return std::unexpected(FileError::UnrecognizedFormat);

			// Read data
			fs.seekg(header.data_offset, std::ios::beg);
			ReadData(fs, file, _authKey);

			return file;
		}
		catch (...)
		{
			return std::unexpected(FileError::ReadError);
		}
	}
}