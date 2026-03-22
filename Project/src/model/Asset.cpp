#include <pch.h>
#include "model/Asset.h"
#include "model/UserProfile.h"
#include "util/Security.h"
#include "util/Common.h"

#include <sstream>
#include <chrono>
#include <cassert>
#include <limits>
#include <crc32.h>

using namespace fig::io::data;

namespace fig::io
{
	fig::string AssetTypeToString(AssetType type, uint8_t subtype)
	{
		string strType, strSubtype;
		if (type == AssetType::Character)
			strType = "data/character";
		else if (type == AssetType::Scenario)
			strType = "data/scenario";
		else if (type == AssetType::Concept)
			strType = "data/concept";
		else if (type == AssetType::ChatInstance)
			strType = "data/chat";
		else if (type == AssetType::ChatLog)
			strType = "data/log";
		else if (type == AssetType::Image)
		{
			strType = "image";
			if (subtype == static_cast<uint8_t>(ImageType::ProfileImage))
				strSubtype = "profile";
			else if (subtype == static_cast<uint8_t>(ImageType::CoverImage))
				strSubtype = "cover";
			else if (subtype == static_cast<uint8_t>(ImageType::LargePortrait))
				strSubtype = "portrait";
			else if (subtype == static_cast<uint8_t>(ImageType::SmallPortrait))
				strSubtype = "small";
			else if (subtype == static_cast<uint8_t>(ImageType::Background))
				strSubtype = "background";
			else if (subtype == static_cast<uint8_t>(ImageType::Expression))
				strSubtype = "expression";
		}
		else
			strType = "unknown";

		if (not strSubtype.empty())
			return std::format("{}/{}", strType, strSubtype);
		else
			return strType;
	}

	std::pair<AssetType, uint8_t> AssetTypeFromString(const fig::string& str)
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

		AssetType type;
		uint8_t subtype = 0u;
		if (strType == "data")
		{
			if (strSubtype == "character")
				type = AssetType::Character;
			else if (strSubtype == "scenario")
				type = AssetType::Scenario;
			else if (strSubtype == "concept")
				type = AssetType::Concept;
			else if (strSubtype == "chat")
				type = AssetType::ChatInstance;
			else if (strSubtype == "log")
				type = AssetType::ChatLog;
		}
		else if (strType == "image")
		{
			type = AssetType::Image;
			if (strSubtype == "profile")
				subtype = static_cast<uint8_t>(ImageType::ProfileImage);
			else if (strSubtype == "cover")
				subtype = static_cast<uint8_t>(ImageType::CoverImage);
			else if (strSubtype == "portrait")
				subtype = static_cast<uint8_t>(ImageType::LargePortrait);
			else if (strSubtype == "small")
				subtype = static_cast<uint8_t>(ImageType::SmallPortrait);
			else if (strSubtype == "background")
				subtype = static_cast<uint8_t>(ImageType::Background);
			else if (strSubtype == "expression")
				subtype = static_cast<uint8_t>(ImageType::Expression);
		}
		else
			type = AssetType::Undefined;

		return std::make_pair(type, subtype);
	}

	fig::string DataFormatToString(DataFormat format)
	{
		switch (format)
		{
		case DataFormat::Text:				return "text/default";
		case DataFormat::DataXml:			return "text/xml";
		case DataFormat::DataJson:			return "text/json";
		case DataFormat::ImageUncompressed:	return "image/bitmap";
		case DataFormat::ImageJpeg:			return "image/jpeg";
		case DataFormat::ImagePng:			return "image/png";
		case DataFormat::ImageWebp:			return "image/webp";
		default:							return "unknown";
		}
	}

	DataFormat DataFormatFromString(const fig::string& str)
	{
		if (str == "text/default")			return DataFormat::Text;
		else if (str == "text/xml")			return DataFormat::DataXml;
		else if (str == "text/json")		return DataFormat::DataJson;
		else if (str == "image/bitmap")		return DataFormat::ImageUncompressed;
		else if (str == "image/jpeg")		return DataFormat::ImageJpeg;
		else if (str == "image/png")		return DataFormat::ImagePng;
		else if (str == "image/webp")		return DataFormat::ImageWebp;
		else								return DataFormat::Undefined;
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

	void Asset::SetData(fig::bytes&& data)
	{
		this->data = std::move(data);
		Invalidate();
	}

	void Asset::SetData(fig::byte_span data)
	{
		this->data.resize(data.size());
		std::memcpy(this->data.data(), data.data(), data.size());
		Invalidate();
	}

	fig::string Asset::AsString() const
	{
		fig::string str;
		str.assign(reinterpret_cast<const char*>(data.data()), data.size());
		return str; // rvo
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

	void Asset::FromFile(const AssetFile& file) noexcept
	{
		id = file.asset_id;
		parent_id = file.parent_id;
		asset_type = static_cast<AssetType>(file.asset_type);
		asset_subtype = file.asset_subtype;
		data_format = static_cast<DataFormat>(file.data_format);
		data = file.data; // copy (for now)
		_parameters = file.meta;
	}

	void Asset::FromFile(AssetFile&& file) noexcept
	{
		id = file.asset_id;
		parent_id = file.parent_id;
		asset_type = static_cast<AssetType>(file.asset_type);
		asset_subtype = file.asset_subtype;
		data_format = static_cast<DataFormat>(file.data_format);
		data = std::move(file.data);
		_parameters = file.meta;
	}

	void Asset::SetMeta(MetaTag tag, bool value) noexcept
	{
		auto meta_type = get_meta_type(tag);
		assert(meta_type == MetaValueType::Boolean);
		_parameters[tag] = value;
		Invalidate();
	}

	void Asset::SetMeta(MetaTag tag, uint8_t value) noexcept
	{
		auto meta_type = get_meta_type(tag);
		assert(meta_type == MetaValueType::UChar);
		_parameters[tag] = value;
		Invalidate();
	}

	void Asset::SetMeta(MetaTag tag, uint16_t value) noexcept
	{
		auto meta_type = get_meta_type(tag);
		assert(meta_type == MetaValueType::UShort);
		_parameters[tag] = value;
		Invalidate();
	}

	void Asset::SetMeta(MetaTag tag, int32_t value) noexcept
	{
		auto meta_type = get_meta_type(tag);
		assert(meta_type == MetaValueType::Integer);
		_parameters[tag] = value;
		Invalidate();
	}

	void Asset::SetMeta(MetaTag tag, float value) noexcept
	{
		auto meta_type = get_meta_type(tag);
		assert(meta_type == MetaValueType::Float);
		_parameters[tag] = value;
		Invalidate();
	}

	void Asset::SetMeta(MetaTag tag, fig::timestamp value) noexcept
	{
		auto meta_type = get_meta_type(tag);
		assert(meta_type == MetaValueType::TimeStamp);
		_parameters[tag] = value;
		Invalidate();
	}

	void Asset::SetMeta(MetaTag tag, const fig::uuid& value) noexcept
	{
		auto meta_type = get_meta_type(tag);
		assert(meta_type == MetaValueType::Identifier);
		_meta_identifier id;
		value.bytes((char*)id.data());
		_parameters[tag] = id;
		Invalidate();
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

	void Asset::Invalidate()
	{
		if (file_status == AssetFileStatus::Invalid || file_status == AssetFileStatus::Missing)
		{
			save_status = AssetSaveStatus::Invalid;
			return;
		}

		file_status = AssetFileStatus::Modified;
		if (save_status != AssetSaveStatus::Created)
			save_status = AssetSaveStatus::Updated;
	}
}