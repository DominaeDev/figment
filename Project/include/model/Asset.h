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
		Undefined			= 0x00,
		Character			= 0x01,
		Scenario			= 0x02,
		Concept				= 0x03,

		Image				= 0x0A,
		ChatInstance		= 0x14,
		ChatLog				= 0x15,

		Reference			= 0x40,
	};

	enum class DataFormat : uint8_t
	{
		Undefined			= 0x00,	// generic binary
		Text				= 0x01,	// utf-8
	
		DataXml				= 0x04,	// utf-8
		DataJson			= 0x05,	// utf-8

		ImageRGB24			= 0x0A,	// bitmap
		ImageARGB32			= 0x0B,	// bitmap
		ImageJpeg			= 0x0C,
		ImagePng			= 0x0D,
		ImageWebp			= 0x0E,
	};

	enum class ImageType : uint8_t
	{
		Unspecified			= 0x00,
		ProfileImage		= 0x01,
		CoverImage			= 0x02,	// card
		SmallPortrait		= 0x03,
		LargePortrait		= 0x04,
		Background			= 0x05,
		Expression			= 0x0A, // ...
	};

	enum class ReferenceType : uint8_t
	{
		Unspecified			= 0x00,
		Original			= 0x01,
	};

	enum class AssetFileStatus : uint8_t
	{
		NotLoaded = 0,
		PartiallyLoaded,
		FullyLoaded,
		Modified,
		Missing,
		Invalid,
	};

	fig::string AssetTypeToString(AssetType type, uint8_t subtype);
	std::pair<AssetType, uint8_t> AssetTypeFromString(const fig::string& str);
	fig::string DataFormatToString(DataFormat format);
	DataFormat DataFormatFromString(const fig::string& str);
	DataFormat DataFormatFromExt(const fig::string& ext);

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
		void SetMeta(MetaTag tag, const fig::uuid& value) noexcept;

		fig::string GetFileName() const noexcept
		{
			return id.str()
				| std::ranges::views::filter([](char c) { return c != '-'; })
				| std::ranges::to<fig::string>();
		}

		AssetFile ToFile() const noexcept;
		void FromFile(const AssetFile& file) noexcept;
		void FromFile(AssetFile&& file) noexcept;

		constexpr bool IsOfType(AssetType type) const noexcept { return asset_type == type; }
		constexpr bool IsOfImageType(ImageType subtype) const noexcept { return asset_type == AssetType::Image and asset_subtype == static_cast<uint8_t>(subtype); }
		constexpr bool IsReference() const noexcept { return asset_type == AssetType::Reference; }
		constexpr bool HasData() const noexcept { return not data.empty(); }
	public:
		fig::uuid id {};
		fig::uuid parent_id {};
		AssetType asset_type { AssetType::Undefined };
		uint8_t asset_subtype {};
		DataFormat data_format { DataFormat::Undefined };
		fig::bytes data {};
		AssetFileStatus status = { AssetFileStatus::NotLoaded };

	private:
		std::map<MetaTag, MetaValue> _parameters {};

	};
}

#endif