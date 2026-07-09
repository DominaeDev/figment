#pragma once

#include "data/Character.h"
#include "data/Scenario.h"
#include "data/CardMetaData.h"
#include "data/ModelSettings.h"

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

		fig::optional_cref<fig::data::Character> GetCharacter(const fig::uuid& id) const noexcept;
		fig::optional_cref<fig::data::Scenario> GetScenario(const fig::uuid& id) const noexcept;
		auto GetCharacters() const noexcept { return _characters | std::views::values; }
		auto GetScenarios() const noexcept { return _scenarios | std::views::values; }
		
		fig::cref_vector<Asset> GetChatsWithCharacter(const fig::uuid& characterId, bool bLoad = false);
		fig::cref_vector<Asset> GetChats(bool bLoad = false);

		std::optional<fig::data::ModelSettings> GetActiveModelSettings() const noexcept;
		fig::optional_ref<fig::data::CardMetaData> GetMetaData(const fig::uuid& id, bool bIgnoreCache = false) noexcept;
		fig::expected_ref<fig::sdl::Texture, FileError> GetSmallPortraitForCharacter(fig::gui::RendererPtr pRenderer, const fig::uuid& characterId) noexcept;
		std::optional<fig::string> GetCharacterName(const fig::uuid& characterId) const;

		bool MarkImported(const fig::uuid& assetId, bool value = true);
		bool MarkFavorite(const fig::uuid& assetId, bool value = true);
		bool MarkHidden(const fig::uuid& assetId, bool value = true);
		bool SetBorder(const fig::uuid& assetId, fig::data::CardBorderStyle borderStyle);

		AssetManager& GetAssetManager();
		void SaveModified();

		size_t GetChatCount(const fig::uuid& assetId);
		void RefreshChatCount();

	protected:
		void LoadAll();

	private:
		std::unique_ptr<fig::io::AssetManager> _pAssetMngr;

		std::map<fig::uuid, fig::data::Character> _characters;
		std::map<fig::uuid, fig::data::Scenario> _scenarios;
		std::map<fig::uuid, fig::data::ChatInstance> _chats;
		std::map<fig::uuid, fig::data::CardMetaData> _metaData;
		std::map<fig::uuid, fig::sdl::Surface> _surfaces;
		std::map<fig::uuid, fig::sdl::Texture> _textures;

		std::map<fig::uuid, std::vector<fig::uuid>> _chatsByAsset; // <asset id, chat ids>

		template <fig::data::CardMetaData::Flag E>
		bool MarkFlag(const fig::uuid& assetId, bool value);
	};
}
