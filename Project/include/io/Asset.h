#pragma once

#include "Figment.h"
#include "io/Serialization.h"
#include "io/ContentUserSettings.h"
#include <map>
#include <variant>

namespace fig::io
{
	enum class AssetType : uint8_t
	{
		Undefined			= 0x00,
		Character			= 0x01,
		Scenario			= 0x02,
		World				= 0x03,
		Concept				= 0x04,

		ModelSettings		= 0x08,

		ChatInstance		= 0x14,
		ChatLog				= 0x15,

		Image				= 0x0A,
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

	struct AssetSyncState
	{
		enum class Status
		{
			Created = 0,		// Modified, nonexisting (INSERT, not UPDATE)
			Modified,			// Modified, existing (UPDATE)
			Synchronized,		// Saved
			Indeterminate,		// Partial or missing data in otherwise valid asset
		};

		enum class Error
		{
			NoError = 0,
			Missing,
			Invalid,
		};

		Status file_sync { Status::Indeterminate };
		Status db_sync { Status::Indeterminate };
		bool has_meta { false };
		bool has_data { false };
		Error error { Error::NoError };

		inline constexpr void InvalidateData() noexcept
		{
			file_sync = AssetSyncState::Status::Modified;
		}

		inline constexpr void InvalidateMetadata() noexcept
		{
			if (db_sync != AssetSyncState::Status::Created)
				db_sync = AssetSyncState::Status::Modified;
		}

		inline constexpr bool ShouldWriteToDisk() const noexcept
		{
			return error == Error::NoError and has_meta and has_data and file_sync < Status::Synchronized;
		}

		inline constexpr bool ShouldWriteToDatabase() const noexcept
		{
			return error == Error::NoError and db_sync < Status::Synchronized;
		}
	};

	fig::string AssetTypeToString(AssetType type, uint8_t subtype) noexcept;
	std::pair<AssetType, uint8_t> AssetTypeFromString(const fig::string& str) noexcept;
	fig::string DataFormatToString(DataFormat format) noexcept;
	DataFormat DataFormatFromString(const fig::string& str) noexcept;
	DataFormat DataFormatFromExt(const fig::string& ext);

	enum class FolderCategory
	{
		Undefined = 0,
		Character,
		Scenario,
	};

	fig::string FolderCategoryToString(FolderCategory format) noexcept;
	FolderCategory FolderCategoryFromString(const fig::string& str) noexcept;

	struct AssetFolder
	{
		fig::uuid id {};
		fig::uuid parent_id {};
		fig::string name {};
		FolderCategory category {};
	};

	class Asset
	{
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

		ContentUserSettings GetUserSettings() const noexcept;
		inline constexpr const fig::string& GetUserSettingsJson() const { return _settings; }
		void SetUserSettings(const ContentUserSettings& settings) noexcept;
		void SetUserSettingsJson(const fig::string& json) noexcept { _settings = json; }

		fig::path GetFileName() const noexcept
		{
			return fig::path(id.to_str()
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

		template <>
		std::optional<fig::uuid> GetMeta(MetaTag tag) const
		{
			if (auto try_get = GetMeta<_meta_identifier>(tag))
				return fig::uuid((*try_get)[1], (*try_get)[0]);
			return std::nullopt;
		}

		constexpr fig::timestamp GetCreatedAt() const noexcept { return GetMeta<fig::timestamp>(MetaTag::CreatedAt).value_or({}); }
		constexpr fig::timestamp GetUpdatedAt() const noexcept { return GetMeta<fig::timestamp>(MetaTag::UpdatedAt).value_or({}); }
		constexpr fig::timestamp GetLastUsedAt() const noexcept { return GetMeta<fig::timestamp>(MetaTag::LastUsedAt).value_or({}); }

		bool HasReferenceTo(const fig::uuid& assetId) const
		{
			for (auto& kvp : _parameters)
			{
				auto key = kvp.first;
				if ((static_cast<uint8_t>(key) >= static_cast<uint8_t>(MetaTag::ReferenceToCharacter) and static_cast<uint8_t>(key) < static_cast<uint8_t>(MetaTag::ReferenceToUser))
					or key == MetaTag::ReferenceToUser
					or key == MetaTag::ReferenceToScenario
					or key == MetaTag::ReferenceToWorld)
				{
					if (GetMeta<fig::uuid>(key) == assetId)
						return true;
				}
			}
			return false;
		}

	private:
		void SetUpdated(bool bWriteTimestamp = true);

	public:
		fig::uuid id {};
		fig::uuid parent_id {};
		fig::uuid folder_id {};
		AssetType asset_type { AssetType::Undefined };
		uint8_t asset_subtype {};
		DataFormat data_format { DataFormat::Undefined };
		fig::bytes data {};
		AssetSyncState sync_state {};

	private:
		fig::string _settings;
		std::map<MetaTag, MetaValue> _parameters {};

	};
}
