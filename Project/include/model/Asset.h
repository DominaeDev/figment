#ifndef ASSET_H__
#define ASSET_H__
#pragma once

#include "Types.h"
#include "util/Serialization.h"
#include <map>
#include <variant>

namespace fig::fs
{
	enum class AssetType : uint8_t
	{
		Undefined = 0,

		Character = 1,
		Scenario = 2,
		Concept = 3,

		Image = 10,

		ChatInstance = 20,
		ChatLog = 21,
	};

	enum class DataFormat : uint8_t
	{
		Undefined	= 0, // generic binary
		Text		= 1, // utf-8
		
		DataXml		= 4, // utf-8
		DataJson	= 5, // utf-8

		ImageJpeg	= 10,
		ImagePng	= 11,
		ImageWebp	= 12,
	};

	enum class ImageSubtype : uint8_t
	{
		Unspecified		= 0,
		ProfileImage	= 1,
		CoverImage		= 2,
		SquarePortrait	= 3,
		LargePortrait	= 4,
		Background		= 5,
		Expression		= 10,
	};

	enum class FileStatus : uint8_t
	{
		NotLoaded = 0,
		PartiallyLoaded,
		FullyLoaded,
		Modified,
		Invalid,
	};

	fig::string AssetTypeToString(AssetType type, uint8_t subtype);
	std::pair<AssetType, uint8_t> AssetTypeFromString(const fig::string& str);
	fig::string DataFormatToString(DataFormat format);
	DataFormat DataFormatFromString(const fig::string& str);

	class Asset
	{
	public:
		void SetData(fig::bytes&& data);
		void SetData(fig::byte_span data);
		constexpr fig::string AsString() const;

		void SetMeta(MetaTag tag, bool value) noexcept;
		void SetMeta(MetaTag tag, int32_t value) noexcept;
		void SetMeta(MetaTag tag, float value) noexcept;
		void SetMeta(MetaTag tag, fig::timestamp value) noexcept;
		void SetMeta(MetaTag tag, const char* value) noexcept;
		void SetMeta(MetaTag tag, const fig::string& value) noexcept;

		fig::string GetFileName() const noexcept
		{
			return id.str()
				| std::ranges::views::filter([](char c) { return c != '-'; })
				| std::ranges::to<fig::string>();
		}

		AssetFile ToFile() const noexcept;
		void FromFile(const AssetFile& file) noexcept;
		void FromFile(AssetFile&& file) noexcept;

		bool IsOfType(AssetType type) const noexcept { return asset_type == type; }
		bool IsOfImageType(ImageSubtype subtype) const noexcept { return asset_type == AssetType::Image and asset_subtype == static_cast<uint8_t>(subtype); }

	public:
		fig::uuid id {};
		fig::uuid parent_id {};
		AssetType asset_type { AssetType::Undefined };
		uint8_t asset_subtype {};
		DataFormat data_format { DataFormat::Undefined };
		fig::bytes data {};
		FileStatus status = { FileStatus::NotLoaded };

	private:
		std::map<MetaTag, MetaValue> _parameters {};

	};
}

#endif