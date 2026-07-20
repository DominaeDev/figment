#pragma once

#include "data/ContentMetaData.h"
#include "io/ContentUserSettings.h"
#include "data/ModelSettings.h"
#include "io/AssetCache.h"

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

		fig::cref_vector<Asset> GetChatLogs(bool bLoad = false);
		fig::cref_vector<Asset> GetChatLogsWith(const fig::uuid& characterId, bool bLoad = false);

		std::optional<fig::data::ModelSettings> GetActiveModelSettings() const noexcept;
		std::optional<fig::string> GetCharacterName(const fig::uuid& characterId) const;
		fig::optional_ref<ContentMetaData> GetMetaData(const fig::uuid& id) noexcept;
		ContentUserSettings GetUserSettings(const fig::uuid& id) noexcept;
		
		fig::expected_ref<fig::sdl::Texture, FileError> GetSmallPortraitForCharacter(const fig::uuid& characterId, fig::gui::TexturePtr pMask, fig::gui::RendererPtr pRenderer) noexcept;
		fig::optional_cref<Asset> FindLastChatWith(const fig::uuid& characterId) const;

		bool MarkImported(const fig::uuid& assetId, bool value = true);
		bool MarkFavorite(const fig::uuid& assetId, bool value = true);
		bool MarkHidden(const fig::uuid& assetId, bool value = true);
		bool SetBorder(const fig::uuid& assetId, CardBorderStyle borderStyle);

		AssetManager& GetAssetManager();
		void SaveModified();

		size_t GetChatCount(const fig::uuid& assetId);
		void RefreshChatCount();

		template <typename T>
		fig::optional_cref<T> Get(const fig::uuid& assetId) noexcept
		{
			return GetCache<T>().Get(assetId);
		}

		fig::expected_cref<fig::sdl::Texture, FileError> GetTexture(const fig::uuid& assetId, struct SDL_Renderer* pRenderer) noexcept;

		template <typename T>
		void InvalidateCache(const fig::uuid& assetId) noexcept
		{
			GetCache<T>().Erase(assetId);
		}

		void InvalidateMeta(const fig::uuid& assetId) noexcept
		{
			_metaData.erase(assetId);
		}

		void InvalidateUserSettings(const fig::uuid& assetId) noexcept
		{
			_userSettings.erase(assetId);
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
		std::map<fig::uuid, ContentMetaData> _metaData;
		std::map<fig::uuid, ContentUserSettings> _userSettings;
		
		struct CachedTexture
		{
			fig::sdl::Texture pTexture;
			fig::gui::TexturePtr pMask {};
		};
		std::map<fig::gui::RendererPtr, std::map<fig::uuid, std::vector<CachedTexture>>> _cachedTextures;

		std::map<fig::uuid, std::vector<fig::uuid>> _chatsByAsset; // <asset id, chat ids>

		template <ContentUserSettings::Flag E>
		bool MarkFlag(const fig::uuid& assetId, bool value);
	};
}
