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
		virtual void Preload() = 0;
		virtual optional_cref<T> Get(const fig::uuid& assetId) = 0;
		virtual optional_ref<T> TryGet(const fig::uuid& assetId) = 0;
		virtual optional_cref<T> TryGet(const fig::uuid& assetId) const = 0;
		virtual std::map<fig::uuid, T>& GetAll() = 0;
		virtual const std::map<fig::uuid, T>& GetAll() const = 0;
	};

	template <typename TAsset>
	inline constexpr AssetType AssetTypeOf = []<bool Flag = false>()
	{
		static_assert(Flag, "no AssetType mapping for this type");
	}();

	template <typename T, AssetType A, DataFormat F, fixed_string LOG>
	class AssetCache : public AssetCacheBase<T>
	{
		using data_type = T;
		static constexpr AssetType asset_type = A;
		static constexpr DataFormat data_format = F;
		static constexpr auto log_name { LOG.c_str() };
	public:
		AssetCache(fig::observer_ptr<AssetManager> pAssetMngr) :
			_pAssetMngr(pAssetMngr)
		{
		}

		void Preload() override
		{
			auto assets = _pAssetMngr->GetAssetsOfType(asset_type);
			auto assetIds = assets
				| std::views::transform([](auto& a) { return a.id; })
				| std::ranges::to<std::vector>();

			if (assetIds.empty())
				return;

			_pAssetMngr->LoadAssetData(assetIds);

			for (auto& asset : assets)
			{
				if (not asset.HasData())
				{
					if (_pAssetMngr->LoadAsset(asset) != FileError::NoError)
						goto fail;
				}

				AssetLoader<data_type> loader;
				if (auto try_load = loader.Load(asset))
				{
					auto [it, _] = _assets.emplace(asset.id, std::move(try_load.value()));

					_pAssetMngr->ReleaseAssetData(asset.id); // Data no longer needed
					LogLn(std::format("Preloaded asset [{}] {} into cache.", log_name, (fig::string)asset.id));
					continue;
				}

			fail:
				LogLn(std::format("Failed to preload asset [{}] {}.", log_name, (fig::string)asset.id));
				assert(false);
			}

		}

		optional_cref<T> Get(const fig::uuid& assetId) override
		{
			if (auto itFind = _assets.find(assetId); itFind != _assets.cend())
				return itFind->second;

			if (auto try_asset = _pAssetMngr->FindAsset(assetId, asset_type))
			{
				auto& asset = *try_asset;

				if (data_format != DataFormat::Undefined and asset.data_format != data_format)
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
					LogLn(std::format("Loaded asset [{}] {} into cache.", log_name, (fig::string)assetId));
					return fig::optional_cref<data_type>((*it).second);
				}
			}

		fail:
			LogLn(std::format("Failed to load asset [{}] {}.", log_name, (fig::string)assetId));
			assert(false);
			return fig::nullref;
		}

		optional_ref<T> TryGet(const fig::uuid& assetId) override
		{
			if (auto itFind = _assets.find(assetId); itFind != _assets.cend())
				return itFind->second;
			return nullref;
		}

		optional_cref<T> TryGet(const fig::uuid& assetId) const override
		{
			if (auto itFind = _assets.find(assetId); itFind != _assets.cend())
				return itFind->second;
			return nullref;
		}

		std::map<fig::uuid, data_type>& GetAll() override
		{
			return _assets;
		}
		
		const std::map<fig::uuid, data_type>& GetAll() const override
		{
			return _assets;
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