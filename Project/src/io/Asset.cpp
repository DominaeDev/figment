#include <pch.h>
#include "io/Asset.h"
#include "user/UserProfile.h"
#include "user/Security.h"

#include <sstream>
#include <chrono>
#include <cassert>
#include <limits>
#include <crc32.h>

namespace fig::io
{

	DataFormat DataFormatFromExt(const fig::string& ext)
	{
		if (not ext.empty())
		{
			if (ext == ".png" || ext == ".apng")
				return DataFormat::ImagePng;
			if (ext == ".jpg" || ext == ".jpeg" || ext == ".jfif")
				return DataFormat::ImageJpeg;
			if (ext == ".webp")
				return DataFormat::ImageWebp;
			if (ext == ".xml")
				return DataFormat::TextXml;
			if (ext == ".json")
				return DataFormat::TextJson;
			if (ext == ".txt")
				return DataFormat::TextDefault;
		}
		return DataFormat::Undefined;
	}

	FolderCategory FolderCategoryFromString(fig::string_view str) noexcept
	{
		if (str == "character")			return FolderCategory::Character;
		else if (str == "scenario")		return FolderCategory::Scenario;
		else							return FolderCategory::Undefined;
	}

	fig::string FolderCategoryToString(FolderCategory category) noexcept
	{
		switch (category)
		{
		case FolderCategory::Character:	return "character";
		case FolderCategory::Scenario:	return "scenario";
		default:						return "unknown";
		}
	}

	void Asset::SetData(fig::bytes&& buf)
	{
		if (not buf.empty())
		{
			data = std::move(buf);
			sync_state.has_data = true;
		}
		else
		{
			data.clear();
			sync_state.has_data = false;
		}
		
		SetUpdated();
	}

	void Asset::SetData(fig::byte_span buf)
	{
		if (not buf.empty())
		{
			data.resize(buf.size());
			std::memcpy(data.data(), buf.data(), buf.size());
			sync_state.has_data = true;
		}
		else
		{
			data.clear();
			sync_state.has_data = false;
		}

		SetUpdated();
	}

	fig::string_view Asset::AsStringView() const
	{
		return fig::string_view { (const char*)data.data(), data.size() };
	}

	AssetFile Asset::ToFile() const noexcept
	{
		auto file = AssetFile {
			.asset_id = id,
			.parent_id = parent_id,
			.type = type,
			.data_length = data.size(),
			.meta = _parameters,
		};

		file.data = data; // copy (for now)
		return file;
	}

	static void copy_parameters(std::map<MetaTag, MetaValue>& dst, const std::map<MetaTag, MetaValue>& src)
	{
		for (auto it = src.cbegin(); it != src.cend(); ++it)
			dst[it->first] = it->second;
	}

	void Asset::FromFile(const AssetFile& file) noexcept
	{
		static_assert(asset_subtype_type<AssetType>);
		static_assert(asset_subtype_type<DataFormat>);

		id = file.asset_id;
		parent_id = file.parent_id;
		type = file.type;
		data = file.data; // copy (for now)
		copy_parameters(_parameters, file.meta);
	}

	void Asset::FromFile(AssetFile&& file) noexcept
	{
		id = file.asset_id;
		parent_id = file.parent_id;
		type = file.type;
		data = std::move(file.data);
		copy_parameters(_parameters, file.meta);
	}

	void Asset::SetMeta(MetaTag tag, bool value) noexcept
	{
		auto meta_type = get_meta_type(tag);
		assert(meta_type == MetaValueType::Boolean);
		_parameters[tag] = value;
	}

	void Asset::SetMeta(MetaTag tag, uint8_t value) noexcept
	{
		auto meta_type = get_meta_type(tag);
		assert(meta_type == MetaValueType::UChar);
		_parameters[tag] = value;
	}

	void Asset::SetMeta(MetaTag tag, uint16_t value) noexcept
	{
		auto meta_type = get_meta_type(tag);
		assert(meta_type == MetaValueType::UShort);
		_parameters[tag] = value;
	}

	void Asset::SetMeta(MetaTag tag, int32_t value) noexcept
	{
		auto meta_type = get_meta_type(tag);
		assert(meta_type == MetaValueType::Integer);
		_parameters[tag] = value;
	}

	void Asset::SetMeta(MetaTag tag, float value) noexcept
	{
		auto meta_type = get_meta_type(tag);
		assert(meta_type == MetaValueType::Float);
		_parameters[tag] = value;
	}

	void Asset::SetMeta(MetaTag tag, fig::timestamp value) noexcept
	{
		auto meta_type = get_meta_type(tag);
		assert(meta_type == MetaValueType::TimeStamp);
		_parameters[tag] = value;
	}

	void Asset::SetMeta(MetaTag tag, const fig::uuid& value) noexcept
	{
		auto meta_type = get_meta_type(tag);
		assert(meta_type == MetaValueType::Identifier);
		_meta_identifier id;
		value.bytes((char*)id.data());
		_parameters[tag] = id;
	}

	void Asset::SetMeta(MetaTag tag, const char* value) noexcept
	{
		SetMeta(tag, fig::string(value));
	}

	void Asset::SetMeta(MetaTag tag, const fig::string& value) noexcept
	{
		auto meta_type = get_meta_type(tag);
		assert(meta_type == MetaValueType::String);

		if (value.length() >= AssetFile::MaxMetaStringLen)
		{
			// Truncate string
			fig::string copy { value };
			copy.resize(AssetFile::MaxMetaStringLen);
			_parameters[tag] = copy;
		}
		else
		{
			_parameters[tag] = value;
		}
	}

	ContentUserSettings Asset::GetUserSettings() const noexcept
	{
		return ContentUserSettings::FromJson(_settings).value_or({});
	}

	void Asset::SetUserSettings(const ContentUserSettings& value) noexcept
	{
		auto json = ContentUserSettings::ToJson(value);
		if (_settings != json)
		{
			_settings = json;
			sync_state.InvalidateMetadata();
		}
	}

	void Asset::CalculateChecksum()
	{
		if (_parameters.contains(MetaTag::Checksum))
			return;

		if (data.size() == 0)
		{
			_parameters.erase(MetaTag::Checksum);
			return;
		}

		SetMeta(MetaTag::Checksum, static_cast<int32_t>(crc32_fast(data.data(), data.size())));
	}

	void Asset::SetUpdated(bool bWriteTimestamp)
	{
		if (sync_state.error != AssetSyncState::Error::NoError)
			return; // Invalid state

		if (bWriteTimestamp)
			SetMeta(MetaTag::UpdatedAt, now());

		sync_state.InvalidateData();
		sync_state.InvalidateMetadata();
	}

	fig::path Asset::GetFileName() const noexcept
	{
		return filename_from_uuid(id);
	}
}