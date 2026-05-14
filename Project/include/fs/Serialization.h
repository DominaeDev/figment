#ifndef SERIALIZATION_H__
#define SERIALIZATION_H__
#pragma once

#include "Types.h"
#include "Constants.h"
#include "util/Security.h"
#include "fs/FileError.h"
#include <variant>
#include <map>
#include <chrono>
#include <expected>

namespace fig::user
{
	struct UserProfile;
}

namespace fig::io
{
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

	constexpr fig::const_string MagicWord = "FIGM";

	enum class FileHeaderFlag : uint8_t
	{
		Encrypted = 1 << 0,
		Checksum = 1 << 1,
	};
	using FileHeaderFlags = EnumFlags<FileHeaderFlag>;

	struct alignas(8) FileHeader
	{
		char magic[4] = { 'F','I','G','M' };
		uint8_t header_version { 1 };
		FileHeaderFlags flags;
		uint16_t data_offset;
		uint64_t asset_id[2];
		uint64_t parent_id[2];
		uint32_t data_length;
		uint8_t asset_type;
		uint8_t asset_subtype;
		uint8_t data_format;
		uint8_t meta_count;
	};

	enum class MetaTag : uint8_t
	{
		Unknown = 0x00,
		CreatedAt = 0x01,				// fig::timestamp; utc
		UpdatedAt = 0x02,				// fig::timestamp; utc
		Version = 0x03,					// uint8_t
		Checksum = 0x04,				// int32_t
		LastUsedAt = 0x05,				// fig::timestamp; utc

		ImageWidth = 0x10,				// uint16_t
		ImageHeight = 0x11,				// uint16_t
		ImageFormat = 0x12,				// uint8_t

		ReferenceToOriginal = 0x40,		// fig::uuid
	};

	using _meta_identifier = std::array<uint64_t, 2>;
	using MetaValue = std::variant<bool, uint8_t, uint16_t, int32_t, float, fig::string, fig::timestamp, _meta_identifier>;

	enum class MetaValueType : uint8_t
	{
		Boolean = 0x00,
		UChar = 0x01,
		UShort = 0x02,
		Integer = 0x03,
		Float = 0x04,

		String = 0x08,
		TimeStamp = 0x10,
		Identifier = 0x40,

		Unknown = 0xFF,
	};

	constexpr MetaValueType get_meta_type(MetaTag tag) noexcept
	{
		switch (tag)
		{
		case MetaTag::Version:
		case MetaTag::ImageFormat:
			return MetaValueType::UChar;

		case MetaTag::ImageWidth:
		case MetaTag::ImageHeight:
			return MetaValueType::UShort;

		case MetaTag::CreatedAt:
		case MetaTag::UpdatedAt:
		case MetaTag::LastUsedAt:
			return MetaValueType::TimeStamp;

		case MetaTag::ReferenceToOriginal:
			return MetaValueType::Identifier;

		case MetaTag::Checksum:
			return MetaValueType::Integer;

		default:
			return MetaValueType::Unknown;
		}
	}

	struct AssetFile
	{
		fig::uuid asset_id {};
		fig::uuid parent_id {};
		uint8_t asset_type { 0 };
		uint8_t asset_subtype { 0 };
		uint8_t data_format { 0 };

		size_t data_length {};
		bool data_encrypted { true };
		fig::bytes data {};

		std::map<MetaTag, MetaValue> meta {};

		bool has_meta(MetaTag tag) const
		{
			return meta.contains(tag);
		}

		template <typename T>
		std::optional<T> get_meta(MetaTag tag) const
		{
			auto it = meta.find(tag);
			if (it != meta.cend())
				return std::make_optional(std::get<T>(it->second));
			return std::nullopt;
		}

		template <typename T>
		bool try_get_meta(MetaTag tag, T& out_value) const
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
			fig::timestamp value;
			if (try_get_meta(MetaTag::CreatedAt, value))
				return value;
			return fig::timestamp { 0 };
		}

		fig::timestamp GetUpdatedAt() const noexcept
		{
			fig::timestamp value;
			if (try_get_meta(MetaTag::UpdatedAt, value))
				return value;
			return fig::timestamp { 0 };
		}

		fig::timestamp GetLastUsedAt() const noexcept
		{
			fig::timestamp value;
			if (try_get_meta(MetaTag::LastUsedAt, value))
				return value;
			return fig::timestamp { 0 };
		}

		fig::string GetFileName() const noexcept
		{
			return std::format("{0}.{1}",
				asset_id.to_str()
				| std::ranges::views::filter([](char c) { return c != '-'; })
				| std::ranges::to<fig::string>(),
				fig::string(Constants::Paths::AssetFileExt));
		}

		static constexpr size_t MaxMetaStringLen { std::numeric_limits<uint8_t>::max() - 1 };
	};

	struct alignas(8) RecoveryFile
	{
		char magic[4] = { 'F', 'I', 'G', 'R' };
		uint8_t recovery_version { 0 };
		uint8_t reserved { 0 };
		uint16_t profile_version { 0 };
		uint64_t profile_id[2];

		fig::auth::AuthChallenge recovery_challenge {};
		fig::auth::AuthChallenge auth_challenge {};
		fig::auth::AuthSalt auth_salt {};
	};
}

#endif
