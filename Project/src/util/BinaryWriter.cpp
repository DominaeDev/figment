#include <pch.h>
#include "util/BinaryWriter.h"
#include "util/Security.h"
#include <cassert>

namespace fig::fs
{
	BinaryWriter::BinaryWriter(fig::security::AuthKey key) noexcept :
		_authKey { key }
	{
	}

	static uint16_t get_data_offset(const AssetFile& file)
	{
		if (file.IsReference())
			return static_cast<uint16_t>(0xFFFF); // No data

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
			else if (const _meta_identifier* x = std::get_if<_meta_identifier>(&value))
				offset += sizeof(_meta_identifier);
			else if (const fig::string* s = std::get_if<fig::string>(&value))
			{
				offset += sizeof(uint16_t); // length
				offset += static_cast<uint16_t>(s->size()); // data
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
			.data_format = file.data_format,
			.asset_type = file.asset_type,
			.asset_subtype = file.asset_subtype,
		};
		file.asset_id.bytes(reinterpret_cast<char*>(&header.asset_id));
		file.parent_id.bytes(reinterpret_cast<char*>(&header.parent_id));

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
			else if (const _meta_identifier* t = std::get_if<_meta_identifier>(&value))
			{
				fs << tag;
				fs.write((const char*)(t), sizeof(_meta_identifier));
			}
			else if (const fig::string* s = std::get_if<fig::string>(&value))
			{
				fs << tag;
				uint16_t length = static_cast<uint16_t>(s->size());
				fs.write((const char*)(&length), sizeof(uint16_t));
				fs.write(s->c_str(), s->size());
			}
		}
	}

	static void WriteData(std::ofstream& fs, const AssetFile& file, fig::security::AuthKey authKey) noexcept
	{
		if (not file.IsReference())
			fig::security::Encrypt(fs, file.data, authKey);
	}

	FileError BinaryWriter::WriteFile(const fig::path& directory, const AssetFile& file) noexcept
	{
		auto const path = directory / file.GetFileName();

		// Create subfolder
		auto const parentPath = path.parent_path();
		if (not std::filesystem::exists(parentPath))
			std::filesystem::create_directory(parentPath);

		std::ofstream fs(path.wstring(), std::ios::binary | std::ios::out | std::ios::trunc);
		if (not fs.is_open())
			return FileError::FileNotFound;

		WriteHeader(fs, file);
		WriteMeta(fs, file);
		WriteData(fs, file, _authKey);

		return FileError::NoError;
	}
}