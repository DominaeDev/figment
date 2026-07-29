#pragma once

#include "Figment.h"
#include "io/Asset.h"
#include "io/ContentTypes.h"

namespace fig::io
{
	class IAssetCache
	{
	public:
		virtual ~IAssetCache() = default;
		virtual bool Erase(const fig::uuid& id) = 0;
		virtual void Clear() = 0;
	};

	template <typename T>
	class AssetCacheBase : public IAssetCache
	{
	public:
		virtual void Preload() = 0;
		virtual void Insert(const fig::uuid& assetId, T&& value) = 0;
		virtual optional_cref<T> Get(const fig::uuid& assetId) = 0;
		virtual optional_ref<T> TryGet(const fig::uuid& assetId) = 0;
		virtual optional_cref<T> TryGet(const fig::uuid& assetId) const = 0;
		virtual std::map<fig::uuid, T>& GetAll() = 0;
		virtual const std::map<fig::uuid, T>& GetAll() const = 0;
	};

	template <typename T, fixed_string LOG>
	class AssetCache : public AssetCacheBase<T>
	{
		using data_type = T;
		static constexpr AssetTypeDefinition _asset_type_definition = AssetTypeOf<T>;
		static constexpr AssetType _asset_type { _asset_type_definition.type };
		static constexpr DataFormat _data_format { _asset_type_definition.format };
		static constexpr auto _log_name { LOG.c_str() };

	public:
		AssetCache(fig::observer_ptr<AssetManager> pAssetMngr) :
			_pAssetMngr(pAssetMngr)
		{
		}

		void Preload() override
		{
			auto assets = _pAssetMngr->GetAssetsOfType(_asset_type);
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
					_assets.emplace(asset.id, std::move(try_load.value()));
					_pAssetMngr->ReleaseAssetData(asset.id); // Data no longer needed
					LogLn(std::format("Preloaded asset [{}] {} into cache.", _log_name, (fig::string)asset.id));
					continue;
				}

			fail:
				LogLn(std::format("Failed to preload asset [{}] {}.", _log_name, (fig::string)asset.id));
				assert(false && "Failed to preload asset");
			}

		}

		optional_cref<T> Get(const fig::uuid& assetId) override
		{
			if (auto itCache = _assets.find(assetId); itCache != _assets.cend())
				return itCache->second;

			if (auto try_asset = _pAssetMngr->FindAsset(assetId, _asset_type))
			{
				auto& asset = *try_asset;

				if (_data_format != DataFormat::Undefined and !asset.type.IsFormat(_data_format))
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
					LogLn(std::format("Loaded asset [{}] {} into cache.", _log_name, (fig::string)assetId));
					return fig::optional_cref<data_type>((*it).second);
				}
			}

		fail:
			LogLn(std::format("Failed to load asset [{}] {}.", _log_name, (fig::string)assetId));
			assert(false && "Failed to load asset");
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

		bool Erase(const fig::uuid& id) override
		{
			return _assets.erase(id) != 0uz;
		}
				
		void Clear()
		{
			_assets.clear();
		}

		void Insert(const fig::uuid& assetId, T&& value)
		{
			_assets.emplace(assetId, std::move(value));
		}

	private:
		observer_ptr<AssetManager> _pAssetMngr;
		std::map<fig::uuid, data_type> _assets;
	};
}