#ifndef CHARACTER_DATABASE_H__
#define CHARACTER_DATABASE_H__
#pragma once

#include "model/CharacterData.h"
#include "model/ScenarioData.h"
#include "model/CardMetaData.h"

namespace fig::io
{
	class AssetManager;

	class UserContentManager
	{
	public:
		UserContentManager(const fig::user::UserProfile& profile, const fig::auth::AuthKey& authKey);
		~UserContentManager();

		size_t ImportCharactersInDirectory(const fig::path& directory);
		std::expected<AssetRef, FileError> ImportCharacter(const fig::path& filename);
		std::expected<AssetRef, FileError> ImportScenario(const fig::path& filename);

		std::optional<fig::io::CharacterData> GetCharacter(const fig::uuid& id) const noexcept;
		auto GetCharacters() const noexcept { return _characters | std::views::values; }
		std::optional<fig::io::ScenarioData> GetScenario(const fig::uuid& id) const noexcept;
		auto GetScenarios() const noexcept { return _scenarios | std::views::values; }

		std::optional<CardMetaDataRef> GetMetaData(const fig::uuid& id, bool bIgnoreCache = false) noexcept;

		bool MarkNew(const fig::uuid& assetId, bool value = true);
		bool MarkImported(const fig::uuid& assetId, bool value = true);
		bool MarkFavorite(const fig::uuid& assetId, bool value = true);
		bool MarkHidden(const fig::uuid& assetId, bool value = true);
		bool SetBorder(const fig::uuid& assetId, CardBorderStyle borderStyle);

		AssetManager& GetAssetManager();
		void SaveModified();

	protected:
		void LoadAll();

	private:
		std::unique_ptr<fig::io::AssetManager> _pAssetMngr;

		std::map<fig::uuid, fig::io::CharacterData> _characters;
		std::map<fig::uuid, fig::io::ScenarioData> _scenarios;
		std::map<fig::uuid, fig::io::CardMetaData> _metaData;

		template <CardMetaData::Flag E>
		bool MarkFlag(const fig::uuid& assetId, bool value);
	};
}
#endif