#include <pch.h>
#include "model/Asset.h"
#include "model/UserProfile.h"
#include "util/Security.h"
#include "util/Common.h"

#include <sstream>
#include <chrono>
#include <cassert>

namespace fig::fs
{

	fig::string AssetTypeToString(AssetType type, uint8_t subtype)
	{
		string strType, strSubtype;
		if (type == AssetType::Character)
			strType = "character";
		else if (type == AssetType::Scenario)
			strType = "scenario";
		else if (type == AssetType::Concept)
			strType = "concept";
		else if (type == AssetType::ChatInstance)
			strType = "chat";
		else if (type == AssetType::ChatLog)
			strType = "chat/log";
		else if (type == AssetType::Image)
		{
			strType = "image";
			if (subtype == static_cast<uint8_t>(ImageSubtype::ProfileImage))
				strSubtype = "profile";
			else if (subtype == static_cast<uint8_t>(ImageSubtype::CoverImage))
				strSubtype = "cover";
			else if (subtype == static_cast<uint8_t>(ImageSubtype::LargePortrait))
				strSubtype = "portrait";
			else if (subtype == static_cast<uint8_t>(ImageSubtype::SquarePortrait))
				strSubtype = "square";
			else if (subtype == static_cast<uint8_t>(ImageSubtype::Background))
				strSubtype = "background";
			else if (subtype == static_cast<uint8_t>(ImageSubtype::Expression))
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
		if (strType == "character")
			type = AssetType::Character;
		else if (strType == "scenario")
			type = AssetType::Scenario;
		else if (strType == "concept")
			type = AssetType::Concept;
		else if (strType == "chat")
			type = AssetType::ChatInstance;
		else if (strType == "log")
			type = AssetType::ChatLog;
		else if (strType == "image")
		{
			type = AssetType::Image;
			if (strSubtype == "profile")
				subtype = static_cast<uint8_t>(ImageSubtype::ProfileImage);
			else if (strSubtype == "cover")
				subtype = static_cast<uint8_t>(ImageSubtype::CoverImage);
			else if (strSubtype == "portrait")
				subtype = static_cast<uint8_t>(ImageSubtype::LargePortrait);
			else if (strSubtype == "square")
				subtype = static_cast<uint8_t>(ImageSubtype::SquarePortrait);
			else if (strSubtype == "background")
				subtype = static_cast<uint8_t>(ImageSubtype::Background);
			else if (strSubtype == "expression")
				subtype = static_cast<uint8_t>(ImageSubtype::Expression);
		}
		else
			type = AssetType::Undefined;

		return std::make_pair(type, subtype);
	}

	fig::string DataFormatToString(DataFormat format)
	{
		switch (format)
		{
		case DataFormat::Text:			return "text/default";
		case DataFormat::DataXml:		return "text/xml";
		case DataFormat::DataJson:		return "text/json";
		case DataFormat::ImageJpeg:		return "image/jpeg";
		case DataFormat::ImagePng:		return "image/png";
		case DataFormat::ImageWebp:		return "image/webp";
		default:						return "unknown";
		}
	}

	DataFormat DataFormatFromString(const fig::string& str)
	{
		if (str == "text/default")			return DataFormat::Text;
		else if (str == "text/xml")			return DataFormat::DataXml;
		else if (str == "text/json")		return DataFormat::DataJson;
		else if (str == "image/jpeg")		return DataFormat::ImageJpeg;
		else if (str == "image/png")		return DataFormat::ImagePng;
		else if (str == "image/webp")		return DataFormat::ImageWebp;
		else								return DataFormat::Undefined;
	}

	void Asset::SetData(fig::bytes&& data)
	{
		this->data = std::move(data);
		status = AssetFileStatus::Modified;
	}

	void Asset::SetData(fig::byte_span data)
	{
		this->data.resize(data.size());
		std::memcpy(this->data.data(), data.data(), data.size());
		status = AssetFileStatus::Modified;
	}

	constexpr fig::string Asset::AsString() const
	{
		fig::string str;
		str.assign(reinterpret_cast<const char*>(data.data()), data.size());
		return str;
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

	void Asset::SetMeta(MetaTag tag, const char* value) noexcept
	{
		SetMeta(tag, fig::string(value));
	}

	void Asset::SetMeta(MetaTag tag, const fig::string& value) noexcept
	{
		auto meta_type = get_meta_type(tag);
		assert(meta_type == MetaValueType::String);
		_parameters[tag] = value;
	}

}