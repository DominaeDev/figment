#ifndef ASSET_MANAGER_H__
#define ASSET_MANAGER_H__
#pragma once

#include "model/Asset.h"
#include "util/Security.h"
#include <expected>

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
		Asset& CreateAsset(AssetType type, fig::bytes&& data, const fig::uuid& parent = {}) noexcept;
		Asset& CreateAsset(AssetType type, fig::byte_span data, const fig::uuid& parent = {}) noexcept;

		std::optional<AssetRef> FindAsset(const fig::uuid& id) noexcept;
		std::expected<AssetRef, FileError> LoadAsset(const fig::uuid& id) noexcept;
		auto GetAssets() const noexcept { return _assets | std::views::values; }

		bool LoadIndex();
		bool SaveIndex() const;
		void SaveAll();

	private:
		bool LoadAssetMetaData();
		bool WriteAsset(const Asset& asset) const;

	private:
		fig::uuid _profileID;
		fig::path _profilePath;
		fig::security::AuthKey _profileAuthKey {};
		std::map<fig::uuid, Asset> _assets {};
	};

}
#endif