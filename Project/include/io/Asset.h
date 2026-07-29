#pragma once

#include "Figment.h"
#include "io/Serialization.h"
#include "io/ContentUserSettings.h"
#include "io/AssetTypeDefinition.h"

namespace fig::io
{
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

	DataFormat DataFormatFromExt(const fig::string& ext);

	enum class FolderCategory
	{
		Undefined = 0,
		Character,
		Scenario,
	};

	fig::string FolderCategoryToString(FolderCategory format) noexcept;
	FolderCategory FolderCategoryFromString(fig::string_view str) noexcept;

	struct AssetFolder
	{
		fig::uuid id {};
		fig::uuid parent_id {};
		fig::string name {};
		FolderCategory category {};
		fig::string settings {};
	};

	// In-memory representation of a generic asset
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

		constexpr bool IsOfType(AssetType assetType) const noexcept { return type.IsOfType(assetType); }
		constexpr bool IsOfImageType(ImageAssetType subtype) const noexcept { return type.IsOfType(AssetType::Image, subtype); }
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
					if (auto ref = GetMeta<fig::uuid>(key); ref.has_value() and ref.value() == assetId)
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
		AssetTypeDefinition type;
		fig::bytes data {};
		AssetSyncState sync_state {};

	private:
		fig::string _settings;
		std::map<MetaTag, MetaValue> _parameters {};

	};
}
