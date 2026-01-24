#ifndef ASSET_MANAGER_H__
#define ASSET_MANAGER_H__
#pragma once

#include "model/Asset.h"
#include "model/UserProfile.h"

namespace fig::fs
{
	class AssetManager
	{
	public:
		Asset& CreateAsset(AssetType type, fig::bytes&& data) noexcept;
		Asset& CreateAsset(AssetType type, fig::byte_span data) noexcept;

		std::optional<std::reference_wrapper<Asset>> FindAsset(const fig::uuid& id) noexcept;

		void SaveAll();

	private:
		bool WriteAsset(const Asset& asset) const;

	private:
		std::map<fig::uuid, Asset> _assets {};
	};

}
#endif