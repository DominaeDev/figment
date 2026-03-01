#ifndef ASSET_MANAGER_H__
#define ASSET_MANAGER_H__
#pragma once

#include "model/Asset.h"
#include "util/Security.h"
#include <expected>
#include <ranges>

namespace fig::data
{
	class CharacterData;
}

namespace fig::fs
{
	class UserManager;

	using AssetRef = std::reference_wrapper<Asset>;
	class AssetManager
	{
		AssetManager() = delete;
	public:
		AssetManager(const UserManager& userMngr);

		Asset& CreateEmptyAsset(AssetType type, const fig::uuid& parent = {}) noexcept;
		Asset& CreateAsset(AssetType type, DataFormat format, fig::bytes&& data, const fig::uuid& parent = {}) noexcept;
		Asset& CreateAsset(AssetType type, DataFormat format, fig::byte_span data, const fig::uuid& parent = {}) noexcept;
		Asset& CreateImageAsset(ImageType subtype, DataFormat format, fig::bytes&& data, const fig::uuid& parent = {}) noexcept;
		Asset& CreateImageAsset(ImageType subtype, DataFormat format, fig::byte_span data, const fig::uuid& parent = {}) noexcept;
		Asset& CreateImageAsset(ImageType subtype, const fig::sdl::Surface& surface, const fig::uuid& parent) noexcept;

		uint32_t DeleteAsset(const fig::uuid& assetID) noexcept;
		uint32_t DeleteAssets(std::span<fig::uuid> assetIDs) noexcept;

		std::optional<AssetRef> FindAsset(const fig::uuid& id) noexcept;
		FileError LoadAsset(const Asset& asset) noexcept;
		std::expected<AssetRef, FileError> LoadAsset(const fig::uuid& id) noexcept;
		
		auto GetAssets() const noexcept { return _assets | std::views::values; }
		auto GetAllCharacters() const noexcept { return _assets | std::views::values | std::views::filter([](auto& a) { return a.asset_type == fig::fs::AssetType::Character; }); }
		auto GetAllScenarios() const noexcept { return _assets | std::views::values | std::views::filter([](auto& a) { return a.asset_type == fig::fs::AssetType::Scenario; }); }

		std::optional<AssetRef> FindAsset(const fig::uuid& parentId, ImageType imageType) noexcept;

		void SaveModified();

		enum class CharacterDataFormat { Default, TavernV2, };
		void ImportCharacters(fig::path directory);
		std::expected<fig::data::CharacterData, FileError> ImportCharacter(fig::path filename, CharacterDataFormat format = CharacterDataFormat::Default);
		std::expected<AssetRef, FileError> ImportScenario(fig::path filename);

	private:
		fig::uuid NewUUID() const noexcept;
		
		bool LoadIndex();
		bool SaveIndex() const;
		bool LoadAssetMetaData();
		bool WriteAsset(const Asset& asset) const;
		bool EraseAsset(const fig::uuid& assetID) noexcept;

	private:
		fig::uuid _profileID;
		fig::path _profilePath;
		fig::security::AuthKey _profileAuthKey {};
		std::map<fig::uuid, Asset> _assets {};
	};

}
#endif