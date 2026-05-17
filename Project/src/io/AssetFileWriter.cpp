#include <pch.h>
#include "io/AssetFileWriter.h"
#include "user/UserProfile.h"
#include "io/Asset.h"
#include <Crc32.h>
#include <cassert>

namespace fig::io
{
	AssetFileWriter::AssetFileWriter(const fig::path& directory) noexcept :
		_directory { directory }
	{
	}

	AssetFileWriter::AssetFileWriter(const fig::path& directory, fig::auth::AuthKey key) noexcept :
		_directory { directory },
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
			else if (const uint8_t* x = std::get_if<uint8_t>(&value))
				offset += sizeof(uint8_t);
			else if (const uint16_t* x = std::get_if<uint16_t>(&value))
				offset += sizeof(uint16_t);
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
				offset += sizeof(uint8_t); // length
				offset += static_cast<uint32_t>(s->size()); // data
			}
		}

		assert(offset <= static_cast<uint16_t>(std::numeric_limits<uint16_t>::max()));
		return offset;
	}

	static void WriteHeader(std::ofstream& fs, const AssetFile& file) noexcept
	{
		FileHeader header {
			.data_length = static_cast<uint32_t>(file.data_length),
			.asset_type = file.asset_type,
			.asset_subtype = file.asset_subtype,
			.data_format = file.data_format,
		};
		file.asset_id.bytes(reinterpret_cast<char*>(&header.asset_id));
		file.parent_id.bytes(reinterpret_cast<char*>(&header.parent_id));

		assert(file.meta.size() <= toUZ(std::numeric_limits<uint8_t>::max()));
		header.meta_count = static_cast<uint8_t>(file.meta.size());
		header.data_offset = get_data_offset(file);

		if (file.data_encrypted)
			header.flags = header.flags | FileHeaderFlag::Encrypted;
		if (file.has_meta(MetaTag::Checksum))
			header.flags = header.flags | FileHeaderFlag::Checksum;

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
			else if (const uint8_t* i = std::get_if<uint8_t>(&value))
			{
				fs << tag;
				fs.write((const char*)(i), sizeof(uint8_t));
			}
			else if (const uint16_t* i = std::get_if<uint16_t>(&value))
			{
				fs << tag;
				fs.write((const char*)(i), sizeof(uint16_t));
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
				fs << static_cast<uint8_t>(MetaValueType::String);
				uint8_t length = static_cast<uint8_t>(std::min(s->size(), 254uz));
				fs.write((const char*)(&length), sizeof(uint8_t));
				fs.write(s->c_str(), length);
			}
		}
	}

	static void WriteData(std::ofstream& fs, const AssetFile& file, fig::auth::AuthKey authKey) noexcept
	{
		if (file.data.size() == 0uz)
			return; // No data

		if (file.data_encrypted)
		{
			// Write encrypted
			fig::auth::Encrypt(fs, file.data, authKey);
		}
		else
		{
			// Write unencrypted
			fs.write((const char*)file.data.data(), file.data.size());
		}
	}

	FileError AssetFileWriter::WriteFile(const AssetFile& file) noexcept
	{
		auto const path = _directory / file.GetFileName();

		try
		{
			// Create subfolder
			auto const parentPath = path.parent_path();
			if (not std::filesystem::exists(parentPath))
				std::filesystem::create_directory(parentPath);

			std::ofstream fs(path.wstring(), std::ios::binary | std::ios::out | std::ios::trunc);
			if (not fs.is_open())
				return FileError::WriteError;

			WriteHeader(fs, file);
			WriteMeta(fs, file);
			WriteData(fs, file, _authKey);

			return FileError::NoError;
		}
		catch (...)
		{
			return FileError::WriteError;
		}
	}

	FileError AssetFileWriter::WriteRecoveryFile(const fig::user::UserProfile& profile, const fig::auth::AuthChallenge& recoveryChallenge) noexcept
	{
		auto directory = profile.GetPath();
		auto const path = directory / fig::path(std::format("{}.{}", Constants::Paths::RecoveryFileName, Constants::Paths::RecoveryFileExt));

		try
		{
			// Create subfolder
			auto const parentPath = path.parent_path();
			if (not std::filesystem::exists(parentPath))
				std::filesystem::create_directory(parentPath);

			std::ofstream fs(path.wstring(), std::ios::binary | std::ios::out | std::ios::trunc);
			if (not fs.is_open())
				return FileError::WriteError;

			RecoveryFile file {
				.profile_version = profile.version,
				.recovery_challenge = recoveryChallenge,
				.auth_challenge = profile.auth.challenge,
				.auth_salt = profile.auth.salt,
			};
			profile.id.bytes(reinterpret_cast<char*>(&file.profile_id));

			fs.write((const char*)(&file), sizeof(file));
			return FileError::NoError;
		}
		catch (...)
		{
			return FileError::WriteError;
		}
	}

	FileError AssetFileWriter::WriteProfileFile(const fig::user::UserProfile& profile, const fig::path& filename, const fig::io::AssetFile& assetFile) noexcept
	{
		auto directory = profile.GetPath();
		auto const path = directory / filename;

		try
		{
			// Create subfolder
			auto const parentPath = path.parent_path();
			if (not std::filesystem::exists(parentPath))
				std::filesystem::create_directory(parentPath);

			std::ofstream fs(path.wstring(), std::ios::binary | std::ios::out | std::ios::trunc);
			if (not fs.is_open())
				return FileError::WriteError;

			WriteHeader(fs, assetFile);
			WriteMeta(fs, assetFile);
			fs.write((const char*)assetFile.data.data(), assetFile.data.size());
			return FileError::NoError;
		}
		catch (...)
		{
			return FileError::WriteError;
		}
	}
}