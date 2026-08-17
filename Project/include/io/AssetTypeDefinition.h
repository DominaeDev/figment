#pragma once

namespace fig::io
{
	enum class AssetType : uint8_t
	{
		Undefined		= 0x00,
		Character		= 0x01,
		Scenario		= 0x02,
		World			= 0x03, //! @reserved
		Concept			= 0x04, //! @reserved

		ModelSettings	= 0x08,
		Chat			= 0x09,

		Image			= 0x0A,
		Animation		= 0x0B, //! @reserved
		Audio			= 0x0C,
	};

	enum class DataFormat : uint8_t
	{
		Undefined			= 0x00,
		
		TextDefault			= 0x01,	// utf-8
		TextXml				= 0x02,	// utf-8
		TextJson			= 0x03,	// utf-8
		TextYaml			= 0x04,	//! @reserved

		ImageUncompressed	= 0x0A,
		ImageJpeg			= 0x0B,
		ImagePng			= 0x0C,
		ImageWebp			= 0x0D,

		AudioWav			= 0x10,
	};

	enum class ImageAssetType : uint8_t
	{
		Undefined			= 0x00,
		ProfileImage		= 0x01,
		CoverImage			= 0x02,
		SmallPortrait		= 0x03,
		LargePortrait		= 0x04,
		Background			= 0x05,
		Expression			= 0x10, //! @todo: expressions?
	};

	enum class ChatAssetType : uint8_t
	{
		Undefined			= 0x00,
		Instance			= 0x01,
		Log					= 0x02,
	};

	enum class AudioAssetType : uint8_t
	{
		Undefined			= 0x00,
		VoiceSettings		= 0x01,
		VoiceReference		= 0x02,
		VoiceMessage		= 0x03,
	};

	template <typename T>
	concept asset_subtype_type =
		(std::is_enum_v<T> and std::is_same_v<uint8_t, typename std::underlying_type<T>::type>)
		or std::is_same_v<uint8_t, T>;

	struct AssetTypeDefinition
	{
		uint8_t type {};
		uint8_t subtype {};
		uint8_t format {};
		uint8_t reserved_ {};

		static constexpr AssetTypeDefinition FromRaw(uint32_t value)
		{
			return AssetTypeDefinition {
				.type { static_cast<uint8_t>(value) },
				.subtype { static_cast<uint8_t>(value >> 8) },
				.format { static_cast<uint8_t>(value >> 16) },
			};
		}

		constexpr operator uint32_t() const
		{
			return static_cast<uint32_t>(format) << 16
				| static_cast<uint32_t>(subtype) << 8
				| static_cast<uint32_t>(type);
		}

		inline constexpr AssetType GetType() const
		{
			return static_cast<AssetType>(type);
		}

		template <asset_subtype_type T>
		inline constexpr T GetSubtype() const
		{
			return static_cast<T>(subtype);
		}

		inline constexpr DataFormat GetFormat() const
		{
			return static_cast<DataFormat>(format);
		}

		inline constexpr bool IsOfType(AssetType type) const noexcept
		{
			return type == GetType();
		}

		template <asset_subtype_type T>
		inline constexpr bool IsOfType(AssetType type, T subtype) const noexcept
		{
			return type == GetType() and (subtype == T {} or subtype == GetSubtype<T>());
		}

		template <asset_subtype_type T>
		inline constexpr bool IsOfType(AssetType type, T subtype, DataFormat format) const noexcept
		{
			return type == GetType() and (subtype == T {} or subtype == GetSubtype<T>()) and format == GetFormat();
		}

		inline constexpr bool IsOfType(AssetType type, DataFormat format) const noexcept
		{
			return type == GetType() and format == GetFormat();
		}

		inline constexpr bool IsOfType(AssetTypeDefinition other, bool exact = false) const noexcept
		{
			return type == other.type
				and (exact ?
					(subtype == other.subtype and format == other.format) :
					(other.subtype == 0 or subtype == other.subtype) and (other.format == 0 or format == other.subtype));
		}

		inline constexpr bool IsFormat(DataFormat format) const noexcept
		{
			return format == GetFormat();
		}

		explicit operator uint32_t() noexcept
		{
			return *reinterpret_cast<uint32_t*>(this);
		}
	};

	inline constexpr AssetTypeDefinition make_asset_type(AssetType asset_type)
	{
		return AssetTypeDefinition {
			.type = static_cast<uint8_t>(asset_type),
		};
	}

	template<asset_subtype_type T>
	inline constexpr AssetTypeDefinition make_asset_type(AssetType asset_type, T subtype)
	{
		return AssetTypeDefinition {
			.type = static_cast<uint8_t>(asset_type),
			.subtype = static_cast<uint8_t>(subtype),
		};
	}

	inline constexpr AssetTypeDefinition make_asset_type(AssetType asset_type, DataFormat format)
	{
		return AssetTypeDefinition {
			.type = static_cast<uint8_t>(asset_type),
			.format = static_cast<uint8_t>(format),
		};
	}

	template<asset_subtype_type T>
	inline constexpr AssetTypeDefinition make_asset_type(AssetType asset_type, T subtype, DataFormat format)
	{
		AssetTypeDefinition type;
		type.type = static_cast<uint8_t>(asset_type);
		type.subtype = static_cast<uint8_t>(subtype);
		type.format = static_cast<uint8_t>(format);
		return type;
	}
}

template <>
struct std::hash<fig::io::AssetTypeDefinition>
{
	size_t operator()(fig::io::AssetTypeDefinition value) const noexcept
	{
		return std::hash<uint32_t>{}(value);
	}
};