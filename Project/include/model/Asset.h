#ifndef ASSET_H__
#define ASSET_H__
#pragma once

#include "Types.h"
#include "fs/Serialization.h"
#include <map>
#include <variant>

namespace fig::io
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
	};

	enum class DataFormat : uint8_t
	{
		Undefined			= 0x00,	// generic binary
		Text				= 0x01,	// utf-8
	
		DataXml				= 0x04,	// utf-8
		DataJson			= 0x05,	// utf-8

		ImageUncompressed	= 0x0A,	// bitmap
		ImageJpeg			= 0x0B,
		ImagePng			= 0x0C,
		ImageWebp			= 0x0D,
	};

	enum class ImageType : uint8_t
	{
		Undefined			= 0x00,
		ProfileImage		= 0x01,
		CoverImage			= 0x02,	// card
		SmallPortrait		= 0x03,
		LargePortrait		= 0x04,
		Background			= 0x05,
		Expression			= 0x0A, // ...
	};

	enum class AssetFileStatus : uint8_t
	{
		NotLoaded = 0,
		PartiallyLoaded,
		FullyLoaded,
		Missing,
		Invalid,
		Modified,
	};

	enum class AssetSaveStatus : uint8_t
	{
		Created = 0,
		Updated,
		Saved,
		Invalid,
	};

	fig::string AssetTypeToString(AssetType type, uint8_t subtype);
	std::pair<AssetType, uint8_t> AssetTypeFromString(const fig::string& str);
	fig::string DataFormatToString(DataFormat format);
	DataFormat DataFormatFromString(const fig::string& str);
	DataFormat DataFormatFromExt(const fig::string& ext);

	class Asset
	{
		using AssetFile = fig::io::AssetFile;
		using MetaTag = fig::io::MetaTag;
		using MetaValue = fig::io::MetaValue;
	public:
		void SetData(fig::bytes&& data);
		void SetData(fig::byte_span data);

		void SetMeta(MetaTag tag, bool value) noexcept;
		void SetMeta(MetaTag tag, uint8_t value) noexcept;
		void SetMeta(MetaTag tag, uint16_t value) noexcept;
		void SetMeta(MetaTag tag, int32_t value) noexcept;
		void SetMeta(MetaTag tag, float value) noexcept;
		void SetMeta(MetaTag tag, fig::timestamp value) noexcept;
		void SetMeta(MetaTag tag, const char* value) noexcept;
		void SetMeta(MetaTag tag, const fig::string& value) noexcept;
		void SetMeta(MetaTag tag, const fig::uuid& value) noexcept;

		fig::path GetFileName() const noexcept
		{
			return fig::path(id.str()
				| std::ranges::views::filter([](char c) { return c != '-'; })
				| std::ranges::to<fig::string>());
		}

		AssetFile ToFile() const noexcept;
		void FromFile(const AssetFile& file) noexcept;
		void FromFile(AssetFile&& file) noexcept;
		void CalculateChecksum();

		constexpr bool IsOfType(AssetType type) const noexcept { return asset_type == type; }
		constexpr bool IsOfImageType(ImageType subtype) const noexcept { return asset_type == AssetType::Image and asset_subtype == static_cast<uint8_t>(subtype); }
		constexpr bool HasData() const noexcept { return not data.empty(); }
		fig::string AsString() const;
		fig::string_view AsStringView() const;

		template <typename T>
		std::optional<T> GetMeta(MetaTag tag) const
		{
			auto it = _parameters.find(tag);
			if (it != _parameters.cend())
			{
				if (const T* x = std::get_if<T>(&it->second))
					return std::make_optional(*x);
			}
			return std::nullopt;
		}

		constexpr fig::timestamp GetCreatedAt() const noexcept { return GetMeta<fig::timestamp>(MetaTag::CreatedAt).value_or({}); }
		constexpr fig::timestamp GetUpdatedAt() const noexcept { return GetMeta<fig::timestamp>(MetaTag::UpdatedAt).value_or({}); }
	
	public:
		fig::uuid id {};
		fig::uuid parent_id {};
		AssetType asset_type { AssetType::Undefined };
		uint8_t asset_subtype {};
		DataFormat data_format { DataFormat::Undefined };
		fig::bytes data {};
		AssetFileStatus file_status = { AssetFileStatus::NotLoaded };
		AssetSaveStatus save_status = { AssetSaveStatus::Created };

	private:
		void Invalidate();

	private:
		std::map<MetaTag, MetaValue> _parameters {};

	};

	using AssetRef = std::reference_wrapper<Asset>;
	using AssetCRef = std::reference_wrapper<const Asset>;

}

#endif