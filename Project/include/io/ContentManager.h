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

		size_t ImportCharactersInDirectory(const fig::path& directory, size_t max_count = 0uz);
		fig::expected_ref<Asset, FileError> ImportCharacter(const fig::path& filename);
		fig::expected_ref<Asset, FileError> ImportScenario(const fig::path& filename);

		const Asset& CreateChat(const fig::data::ChatInstance& chatInstance);
		bool DeleteAsset(fig::uuid assetId);

		fig::cref_vector<Asset> GetChatLogs(bool bLoad = false);
		fig::cref_vector<Asset> GetChatLogsWith(const fig::uuid& characterId, bool bLoad = false);
		fig::cref_vector<Asset> GetCharacters() const noexcept;
		fig::cref_vector<Asset> GetScenarios() const noexcept;

		std::optional<fig::data::ModelSettings> GetActiveModelSettings() const noexcept;
		std::optional<fig::string> GetCharacterName(const fig::uuid& characterId) const;
		fig::optional_ref<ContentMetaData> GetMetaData(const fig::uuid& id) noexcept;
		ContentUserSettings GetUserSettings(const fig::uuid& id) noexcept;
		
		fig::optional_cref<Asset> GetLargePortraitForCharacter(const fig::uuid& characterId) const;
		fig::expected_ref<fig::sdl::Texture, FileError> GetSmallPortraitForCharacter(const fig::uuid& characterId, fig::texture_ptr pMask, fig::renderer_ptr pRenderer) noexcept;
		fig::optional_cref<Asset> FindLastChatWith(const fig::uuid& characterId) const;

		bool MarkImported(const fig::uuid& assetId, bool value = true);
		bool MarkFavorite(const fig::uuid& assetId, bool value = true);
		bool MarkHidden(const fig::uuid& assetId, bool value = true);
		bool SetBorder(const fig::uuid& assetId, CardBorderStyle borderStyle);

		AssetManager& GetAssets();

		uint32_t GetChatCount(const fig::uuid& assetId);

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

		void InvalidateAsset(const fig::uuid& assetId) noexcept
		{
			InvalidateMeta(assetId);
			InvalidateCache(assetId);
		}

		void InvalidateCache(const fig::uuid& assetId) noexcept
		{
			for (auto& kvp : _caches)
			{
				if (kvp.second->Erase(assetId))
					return;
			}
		}

		void InvalidateMeta(const fig::uuid& assetId) noexcept
		{
			_metaData.erase(assetId);
		}

		void InvalidateUserSettings(const fig::uuid& assetId) noexcept
		{
			_userSettings.erase(assetId);
		}

		template <typename T>
		void Cache(const fig::uuid& assetId, const T& value)
		{
			T copy { value };
			GetCache<T>().Insert(assetId, std::move(copy));
		}

		template <typename T>
		void Cache(const fig::uuid& assetId, T&& value)
		{
			GetCache<T>().Insert(assetId, std::move(value));
		}

	protected:
		void LoadAll();
		void RefreshChatCount();
		void InvalidateChatCount() { _bInvalidChatCount = true; }

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
		std::unordered_map<AssetTypeDefinition, std::unique_ptr<IAssetCache>> _caches;
		std::map<fig::uuid, ContentMetaData> _metaData;
		std::map<fig::uuid, ContentUserSettings> _userSettings;
		
		struct CachedTexture
		{
			fig::sdl::Texture pTexture;
			fig::texture_ptr pMask {};
		};
		std::map<fig::renderer_ptr, std::map<fig::uuid, std::vector<CachedTexture>>> _cachedTextures;

		std::map<fig::uuid, std::vector<fig::uuid>> _chatsByAsset; // <asset id, chat ids>
		bool _bInvalidChatCount { true };

		template <ContentUserSettings::Flag E>
		bool MarkFlag(const fig::uuid& assetId, bool value);
	};
}
