#ifndef ASSET_MANAGER_H__
#define ASSET_MANAGER_H__
#pragma once

#include "model/Asset.h"
#include "util/Security.h"
#include <filesystem>

namespace fig::fs
{
	class UserManager;

	class AssetManager
	{
		AssetManager() = delete;
	public:
		AssetManager(const UserManager& userMngr);

		Asset& CreateEmptyAsset(AssetType type) noexcept;
		Asset& CreateAsset(AssetType type, fig::bytes&& data) noexcept;
		Asset& CreateAsset(AssetType type, fig::byte_span data) noexcept;

		std::optional<std::reference_wrapper<Asset>> FindAsset(const fig::uuid& id) noexcept;

		bool LoadIndex();
		bool SaveIndex() const;
		void SaveAll();

	private:
		bool WriteAsset(const Asset& asset) const;

	private:
		fig::uuid _profileID;
		std::filesystem::path _profilePath;
		fig::security::AESKey _profileAuthKey {};
		std::map<fig::uuid, Asset> _assets {};
	};

}
#endif