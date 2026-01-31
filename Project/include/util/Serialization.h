#ifndef BINARY_SERIALIZATION_H__
#define BINARY_SERIALIZATION_H__
#pragma once

#include "Types.h"
#include "Constants.h"
#include <variant>
#include <map>
#include <chrono>

namespace fig::fs
{
	enum class FileError : uint32_t
	{
		NoError = 0,
		FileNotFound,
		FileAccessError,
		DirectoryDoesNotExist,
		UnrecognizedFormat,
		ReadError,
		WriteError,
	};

	struct VersionNumber
	{
		uint8_t major { 0 };
		uint8_t minor { 0 };

		VersionNumber(uint8_t maj = 0, uint8_t min = 0)
			: major(maj), minor(min)
		{
		}

		explicit VersionNumber(uint16_t packed)
		{
			from_uint16(packed);
		}

		constexpr uint16_t to_uint16() const
		{
			return (static_cast<uint16_t>(major) << 8) | minor;
		}

		constexpr void from_uint16(uint16_t packed)
		{
			major = static_cast<uint8_t>((packed >> 8) & 0xFF);
			minor = static_cast<uint8_t>(packed & 0xFF);
		}

		constexpr bool operator==(const VersionNumber& other) const
		{
			return major == other.major && minor == other.minor;
		}

		constexpr bool operator!=(const VersionNumber& other) const
		{
			return !(*this == other);
		}

		constexpr bool operator<(const VersionNumber& other) const
		{
			if (major == other.major)
				return minor < other.minor;
			return major < other.major;
		}

		constexpr bool operator<=(const VersionNumber& other) const
		{
			return *this < other || *this == other;
		}

		constexpr bool operator>(const VersionNumber& other) const
		{
			return !(*this <= other);
		}

		constexpr bool operator>=(const VersionNumber& other) const
		{
			return !(*this < other);
		}

		constexpr bool is_valid() const noexcept
		{
			return major != 0 && minor != 0;
		}
	};

	struct alignas(8) FileHeader
	{
		char magic[4] = { 'F','I','G','M' };
		uint16_t fmt_version = { VersionNumber(1, 0).to_uint16() };
		uint16_t data_offset;
		uint64_t parent_id[2];
		uint64_t asset_id[2];
		uint32_t data_length;
		uint8_t data_format;
		uint8_t asset_type;
		uint8_t asset_subtype;
		uint8_t meta_count;
	};

	enum MetaTag : uint8_t
	{
		Unknown = 0,
		CreatedAt = 1,		// utc
		UpdatedAt = 2,		// utc
		Title = 3,

		ImageWidth = 16,
		ImageHeight = 17,
		ImageType = 18,
	};

	using MetaValue = std::variant<bool, int32_t, float, fig::timestamp, fig::string>;

	enum class MetaValueType : uint8_t
	{
		Boolean = 0,
		Integer = 1,
		Float = 2,
		String = 3,
		TimeStamp = 4,

		Unknown = 255,
	};
	
	constexpr MetaValueType get_meta_type(MetaTag tag) noexcept
	{
		switch (tag)
		{
		case MetaTag::ImageWidth:
		case MetaTag::ImageHeight:
		case MetaTag::ImageType:
			return MetaValueType::Integer;

		case MetaTag::CreatedAt:
		case MetaTag::UpdatedAt:
			return MetaValueType::TimeStamp;

		case MetaTag::Title:
			return MetaValueType::String;

		default:
			return MetaValueType::Unknown;
		}
	}

	struct AssetFile
	{
		fig::uuid asset_id {};
		fig::uuid parent_id {};
		uint8_t asset_type {0};
		uint8_t asset_subtype {0};
		uint8_t data_format {0};

		fig::bytes data {};
		size_t data_length {};
		std::map<MetaTag, MetaValue> meta {};

		template <typename T>
		std::optional<T> get_meta(MetaTag tag) const noexcept
		{
			auto it = meta.find(tag);
			if (it != meta.cend())
				return std::make_optional(std::get<T>(it->second));
			return std::nullopt;
		}

		template <typename T>
		bool try_get_meta(MetaTag tag, T& out_value) const noexcept
		{
			if (auto m = get_meta<T>(tag))
			{
				out_value = m.value();
				return true;
			}
			return false;
		}

		fig::timestamp GetCreatedAt()
		{
			fig::timestamp ts;
			if (try_get_meta(MetaTag::CreatedAt, ts))
				return ts;
			return fig::timestamp { 0 };
		}
		
		fig::timestamp GetUpdatedAt()
		{
			fig::timestamp ts;
			if (try_get_meta(MetaTag::UpdatedAt, ts))
				return ts;
			return fig::timestamp { 0 };
		}

		fig::string GetFileName() const noexcept
		{
			return std::format("{0}.{1}", 
				asset_id.str()
					| std::ranges::views::filter([](char c) { return c != '-'; })
					| std::ranges::to<fig::string>(),
				fig::string(Constants::Paths::AssetFileExt));
		}
	};
}

#endif
