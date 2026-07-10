#pragma once

#include "data/CardMetaData.h"
#include "data/ModelSettings.h"
#include "io/AssetCache.h"
#include "io/ContentTypes.h"

namespace fig::data
{
	struct ChatInstance;
}

namespace fig::io
{
	class AssetManager;

	class UserContentManager
	{
	public:
		UserContentManager(const fig::user::UserProfile& profile, const fig::auth::AuthKey& authKey);
		~UserContentManager();

		size_t ImportCharactersInDirectory(const fig::path& directory, size_t max_count = 0uz);
		fig::expected_ref<Asset, FileError> ImportCharacter(const fig::path& filename);
		fig::expected_ref<Asset, FileError> ImportScenario(const fig::path& filename);

		const Asset& CreateAsset(const fig::data::ChatInstance& chatInstance);

		fig::cref_vector<Asset> GetChatsWithCharacter(const fig::uuid& characterId, bool bLoad = false);
		fig::cref_vector<Asset> GetChatLogs(bool bLoad = false);

		std::optional<fig::data::ModelSettings> GetActiveModelSettings() const noexcept;
		fig::optional_ref<fig::data::CardMetaData> GetMetaData(const fig::uuid& id, bool bIgnoreCache = false) noexcept;
		std::optional<fig::string> GetCharacterName(const fig::uuid& characterId) const;
		
		fig::expected_ref<fig::sdl::Texture, FileError> GetSmallPortraitForCharacter(const fig::uuid& characterId, fig::gui::TexturePtr pMask, fig::gui::RendererPtr pRenderer) noexcept;

		bool MarkImported(const fig::uuid& assetId, bool value = true);
		bool MarkFavorite(const fig::uuid& assetId, bool value = true);
		bool MarkHidden(const fig::uuid& assetId, bool value = true);
		bool SetBorder(const fig::uuid& assetId, fig::data::CardBorderStyle borderStyle);

		AssetManager& GetAssetManager();
		void SaveModified();

		size_t GetChatCount(const fig::uuid& assetId);
		void RefreshChatCount();

		template <typename T>
		fig::optional_cref<T> Get(const fig::uuid& assetId) noexcept
		{
			auto& cache = GetCache<T>();
			return cache.Get(assetId);
		}

	protected:
		void LoadAll();

		template <typename T>
		AssetCacheBase<T>& GetCache()
		{
			auto& entry = _caches[AssetTypeOf<T>];
			return static_cast<AssetCacheBase<T>&>(*entry);
		}

		template <typename T>
		const AssetCacheBase<T>& GetCache() const
		{
			auto& entry = _caches.at(AssetTypeOf<T>);
			return static_cast<const AssetCacheBase<T>&>(*entry);
		}
	private:
		std::unique_ptr<fig::io::AssetManager> _pAssetMngr;
		std::unordered_map<AssetType, std::unique_ptr<IAssetCache>> _caches;
		std::map<fig::uuid, fig::data::CardMetaData> _metaData;
		
		struct CachedTexture
		{
			fig::sdl::Texture pTexture;
			fig::gui::TexturePtr pMask {};
		};
		std::map<fig::gui::RendererPtr, std::map<fig::uuid, std::vector<CachedTexture>>> _cachedTextures;

		std::map<fig::uuid, std::vector<fig::uuid>> _chatsByAsset; // <asset id, chat ids>

		template <fig::data::CardMetaData::Flag E>
		bool MarkFlag(const fig::uuid& assetId, bool value);
	};
}
