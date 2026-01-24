#include <pch.h>
#include "util/BinaryWriter.h"
#include "util/Security.h"
#include <filesystem>
#include <cassert>

namespace fig::fs
{
	BinaryWriter::BinaryWriter(const fig::string& profileName, fig::security::AESKey key) noexcept :
		_profileName { profileName },
		_authKey { key }
	{
	}

	static uint16_t get_data_offset(const AssetFile& file)
	{
		uint32_t offset = 0;
		for (auto it = file.meta.cbegin(); it != file.meta.cend(); ++it)
		{
			offset += 1; // Tag byte
			auto& value = it->second;
			if (const bool* x = std::get_if<bool>(&value))
				offset += 1u;
			else if (const int32_t* x = std::get_if<int32_t>(&value))
				offset += sizeof(int32_t);
			else if (const float* x = std::get_if<float>(&value))
				offset += sizeof(float);
			else if (const fig::timestamp* x = std::get_if<fig::timestamp>(&value))
				offset += sizeof(fig::timestamp);
			else if (const fig::string* s = std::get_if<fig::string>(&value))
			{
				offset += sizeof(uint32_t); // length
				offset += static_cast<uint32_t>(s->size()); // data
			}
		}

		offset += static_cast<uint32_t>(sizeof(FileHeader));
		assert(offset < static_cast<uint32_t>(std::numeric_limits<uint16_t>::max()));
		return offset;
	}

	static void WriteHeader(std::ofstream& fs, const AssetFile& file) noexcept
	{
		FileHeader header {
			.data_length = static_cast<uint32_t>(file.data_length),
			.data_format = file.dataFormat,
			.asset_type = file.assetType,
			.asset_subtype = file.assetSubtype,
		};
		file.assetID.bytes(reinterpret_cast<char*>(&header.asset_id));
		file.parentID.bytes(reinterpret_cast<char*>(&header.parent_id));

		assert(file.meta.size() < std::numeric_limits<uint8_t>::max());
		header.meta_count = static_cast<uint8_t>(file.meta.size());
		header.data_offset = get_data_offset(file);

		fs.write((const char*)(&header), sizeof(header));
	}

	static void WriteMeta(std::ofstream& fs, const AssetFile& file) noexcept
	{
		for (auto it = file.meta.cbegin(); it != file.meta.cend(); ++it)
		{
			auto tag = static_cast<uint8_t>(it->first);
			auto& value = it->second;
			if (const bool* b = std::get_if<bool>(&value))
			{
				fs << tag;
				fs << uint8_t(*b ? 1 : 0);
			}
			else if (const int32_t* i = std::get_if<int32_t>(&value))
			{
				fs << tag;
				fs.write((const char*)(i), sizeof(int32_t));
			}
			else if (const float* f = std::get_if<float>(&value))
			{
				fs << tag;
				fs.write((const char*)(f), sizeof(float));
			}
			else if (const fig::timestamp* t = std::get_if<fig::timestamp>(&value))
			{
				fs << tag;
				fs.write((const char*)(t), sizeof(fig::timestamp));
			}
			else if (const fig::string* s = std::get_if<fig::string>(&value))
			{
				fs << tag;
				uint32_t length = static_cast<uint32_t>(s->size());
				fs.write((const char*)(&length), sizeof(uint32_t));
				fs.write(s->c_str(), s->size());
			}
		}
	}

	static void WriteData(std::ofstream& fs, const AssetFile& file, fig::security::AESKey authKey) noexcept
	{
		std::array<uint8_t, 256> buffer { 0 };
		for (size_t i = 0; i < file.data_length; i += 256)
		{
			std::memcpy(buffer.data(), file.data + i, std::min(sizeof(buffer), file.data_length - i));
			fig::security::Encrypt((fig::byte*)(buffer.data()), buffer.size(), authKey);

			fs.write((const char*)buffer.data(), buffer.size());
		}
	}

	FileError BinaryWriter::WriteFile(const AssetFile& file) noexcept
	{
		fig::string filename = std::format("./profiles/{0}/{1}", _profileName, file.assetID.str());
		auto const path = std::filesystem::path(filename.c_str());

		std::ofstream fs(path.generic_wstring(), std::ios::binary | std::ios::out | std::ios::trunc);
		if (not fs.is_open())
			return FileError::FileNotFound;

		WriteHeader(fs, file);
		WriteMeta(fs, file);
		WriteData(fs, file, _authKey);

		return FileError::NoError;
	}
}