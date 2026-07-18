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
	struct AssetTypeInfo
	{
		fig::string_view category;
		fig::string_view subname;
		uint8_t type;
		uint8_t sub;
	};

	static constexpr std::array<AssetTypeInfo, 11> AssetTypes {
		AssetTypeInfo { "data",		"character",		static_cast<uint8_t>(AssetType::Character) },
		AssetTypeInfo { "data",		"scenario",			static_cast<uint8_t>(AssetType::Scenario) },
		AssetTypeInfo { "data",		"chat",				static_cast<uint8_t>(AssetType::ChatInstance) },
		AssetTypeInfo { "data",		"log",				static_cast<uint8_t>(AssetType::ChatLog) },
		AssetTypeInfo { "data",		"model_config",		static_cast<uint8_t>(AssetType::ModelSettings) },
		AssetTypeInfo { "image",	"profile",			static_cast<uint8_t>(AssetType::Image),		static_cast<uint8_t>(ImageType::ProfileImage) },
		AssetTypeInfo { "image",	"cover",			static_cast<uint8_t>(AssetType::Image),		static_cast<uint8_t>(ImageType::CoverImage) },
		AssetTypeInfo { "image",	"portrait",			static_cast<uint8_t>(AssetType::Image),		static_cast<uint8_t>(ImageType::LargePortrait) },
		AssetTypeInfo { "image",	"small",			static_cast<uint8_t>(AssetType::Image),		static_cast<uint8_t>(ImageType::SmallPortrait) },
		AssetTypeInfo { "image",	"background",		static_cast<uint8_t>(AssetType::Image),		static_cast<uint8_t>(ImageType::Background) },
		AssetTypeInfo { "image",	"expression",		static_cast<uint8_t>(AssetType::Image),		static_cast<uint8_t>(ImageType::Expression) },
	};

	using DataFormatInfo = std::pair<fig::string_view, DataFormat>;

	static constexpr std::array<DataFormatInfo, 7> DataFormats {
		DataFormatInfo { "text/default",	DataFormat::Text },
		DataFormatInfo { "text/xml",		DataFormat::DataXml },
		DataFormatInfo { "text/json",		DataFormat::DataJson },
		DataFormatInfo { "image/bitmap",	DataFormat::ImageUncompressed },
		DataFormatInfo { "image/jpeg",		DataFormat::ImageJpeg },
		DataFormatInfo { "image/png",		DataFormat::ImagePng },
		DataFormatInfo { "image/webp",		DataFormat::ImageWebp },
	};

	fig::string AssetTypeToString(AssetType assetType, uint8_t subtype) noexcept
	{
		uint8_t type = static_cast<uint8_t>(assetType);
		if (auto itFind = std::find_if(AssetTypes.cbegin(), AssetTypes.cend(),
			[type, subtype](auto& t) { return t.type == type && t.sub == subtype; }); 
			itFind != AssetTypes.cend())
		{
			auto& assetType = *itFind;
			if (assetType.subname.empty())
				return fig::string { assetType.category };
			else
				return std::format("{}/{}", assetType.category, assetType.subname);
		}
		else
		{
			return "unknown";
		}
	}

	std::pair<AssetType, uint8_t> AssetTypeFromString(const fig::string& str) noexcept
	{
		fig::string strType, strSubtype;
		size_t pos_slash = str.find('/');
		if (pos_slash != 0)
		{
			strSubtype = str.substr(pos_slash + 1);
			strType = str.substr(0, pos_slash);
		}
		else
		{
			strType = str;
		}

		if (auto itFind = std::find_if(AssetTypes.cbegin(), AssetTypes.cend(),
			[&strType, &strSubtype](auto& t) { return t.category == strType && t.subname == strSubtype; }); 
			itFind != AssetTypes.cend())
		{
			return std::make_pair(static_cast<AssetType>(itFind->type), itFind->sub);
		}
		else
		{
			return std::make_pair(AssetType::Undefined, 0);
		}
	}

	fig::string DataFormatToString(DataFormat format) noexcept
	{
		if (auto itFind = std::find_if(DataFormats.cbegin(), DataFormats.cend(),
			[format](auto& f) { return f.second == format; });
			itFind != DataFormats.cend())
		{
			return fig::string { itFind->first };
		}
		else
		{
			return "unknown";
		}
	}

	DataFormat DataFormatFromString(const fig::string& str) noexcept
	{
		if (auto itFind = std::find_if(DataFormats.cbegin(), DataFormats.cend(),
			[&str](auto& f) { return f.first == str; });
			itFind != DataFormats.cend())
		{
			return itFind->second;
		}
		else
		{
			return DataFormat::Undefined;
		}
	}

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
				return DataFormat::DataXml;
			if (ext == ".json")
				return DataFormat::DataJson;
			if (ext == ".txt")
				return DataFormat::Text;
		}
		return DataFormat::Undefined;
	}

	FolderCategory FolderCategoryFromString(const fig::string& str) noexcept
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

	void Asset::SetData(fig::bytes&& data)
	{
		this->data = std::move(data);
		SetUpdated();
	}

	void Asset::SetData(fig::byte_span data)
	{
		this->data.resize(data.size());
		std::memcpy(this->data.data(), data.data(), data.size());
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
			.asset_type = static_cast<uint8_t>(asset_type),
			.asset_subtype = asset_subtype,
			.data_format = static_cast<uint8_t>(data_format),
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
		id = file.asset_id;
		parent_id = file.parent_id;
		asset_type = static_cast<AssetType>(file.asset_type);
		asset_subtype = file.asset_subtype;
		data_format = static_cast<DataFormat>(file.data_format);
		data = file.data; // copy (for now)
		copy_parameters(_parameters, file.meta);
	}

	void Asset::FromFile(AssetFile&& file) noexcept
	{
		id = file.asset_id;
		parent_id = file.parent_id;
		asset_type = static_cast<AssetType>(file.asset_type);
		asset_subtype = file.asset_subtype;
		data_format = static_cast<DataFormat>(file.data_format);
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
	}
}