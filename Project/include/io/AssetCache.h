#pragma once

#include "Figment.h"
#include "io/Asset.h"

namespace fig::io
{
	template <typename T>
	struct AssetLoader
	{
		std::expected<T, FileError> Load(const Asset& asset);
	};

	class IAssetCache
	{
	public:
		virtual ~IAssetCache() = default;
	};

	template <typename T>
	class AssetCacheBase : public IAssetCache
	{
	public:
		virtual optional_cref<T> Get(const fig::uuid& assetId) = 0;
	};

	template <typename TAsset>
	inline constexpr AssetType AssetTypeOf = []<bool Flag = false>()
	{
		static_assert(Flag, "no AssetType mapping for this type");
	}();

	template <typename T, AssetType A, DataFormat F>
	class AssetCache : public AssetCacheBase<T>
	{
		using data_type = T;
		static constexpr AssetType asset_type = A;
		static constexpr DataFormat data_format = F;

	public:
		AssetCache(fig::observer_ptr<AssetManager> pAssetMngr) :
			_pAssetMngr(pAssetMngr)
		{
		}

		optional_cref<T> Get(const fig::uuid& assetId) override
		{
			if (auto itFind = _assets.find(assetId); itFind != _assets.cend())
				return itFind->second;

			if (auto try_asset = _pAssetMngr->FindAsset(assetId, asset_type))
			{
				auto& asset = *try_asset;

				if (asset.data_format != data_format)
					goto fail;

				if (not asset.HasData())
				{
					if (_pAssetMngr->LoadAsset(asset) != FileError::NoError)
						goto fail;
				}

				AssetLoader<data_type> loader;
				if (auto try_load = loader.Load(asset))
				{
					auto [it, _] = _assets.emplace(assetId, std::move(try_load.value()));

					_pAssetMngr->ReleaseAssetData(assetId); // Data no longer needed
					LogLn(std::format("Loaded asset {} into cache.", (fig::string)assetId));
					return fig::optional_cref<data_type>((*it).second);
				}
			}

		fail:
			LogLn(std::format("Failed to load asset {}.", (fig::string)assetId));
			assert(false);
			return fig::nullref;
		}
		
		void Clear()
		{
			_assets.clear();
		}

	private:
		observer_ptr<AssetManager> _pAssetMngr;
		std::map<fig::uuid, data_type> _assets;
	};
}