#ifndef SERIALIZATION_H__
#define SERIALIZATION_H__
#pragma once

#include "Types.h"
#include "Constants.h"
#include "util/Security.h"
#include <variant>
#include <map>
#include <chrono>
#include <expected>

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

	enum class MetaTag : uint8_t
	{
		Unknown		= 0x00,
		CreatedAt	= 0x01,		// utc
		UpdatedAt	= 0x02,		// utc

		ImageWidth	= 0x10,
		ImageHeight	= 0x11,
		ImageType	= 0x12,

		Reference	= 0x40,
	};

	using _meta_identifier = std::array<uint64_t, 2>;
	using MetaValue = std::variant<bool, int32_t, float, fig::string, fig::timestamp, _meta_identifier>;

	enum class MetaValueType : uint8_t
	{
		Boolean		= 0x00,
		Integer		= 0x01,
		Float		= 0x02,
		String		= 0x03,
		TimeStamp	= 0x04,
		Identifier	= 0x40,

		Unknown		= 0xFF,
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

		case MetaTag::Reference:
			return MetaValueType::Identifier;

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

		fig::timestamp GetCreatedAt() const noexcept
		{
			fig::timestamp ts;
			if (try_get_meta(MetaTag::CreatedAt, ts))
				return ts;
			return fig::timestamp { 0 };
		}
		
		fig::timestamp GetUpdatedAt() const noexcept
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

		bool IsReference() const noexcept
		{
			return get_meta<_meta_identifier>(MetaTag::Reference).has_value();
		}
	};

	class BinaryReader
	{
		BinaryReader() = delete;
	public:
		explicit BinaryReader(const fig::path& directory, fig::security::AuthKey key) noexcept;
		std::expected<AssetFile, FileError> ReadFile(const fig::string& filename, bool read_data = true) noexcept;

	private:
		fig::path _profilePath {};
		fig::security::AuthKey _authKey {};
	};

	class BinaryWriter
	{
		BinaryWriter() = delete;
	public:
		explicit BinaryWriter(fig::security::AuthKey key) noexcept;
		FileError WriteFile(const fig::path& directory, const AssetFile& file) noexcept;

	private:
		fig::security::AuthKey _authKey {};
	};
}

#endif
