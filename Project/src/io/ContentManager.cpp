#include <pch.h>
#include "io/ContentManager.h"
#include "io/AssetManager.h"
#include "gui/AppResources.h"
#include "app/AppState.h"

namespace fig::io
{
	UserContentManager::UserContentManager(const fig::user::UserProfile& profile, const fig::auth::AuthKey& authKey)
	{
		_pAssetMngr = std::make_unique<AssetManager>(profile, authKey);

		LoadAll();
	}

	UserContentManager::~UserContentManager()
	{
		_pAssetMngr->SaveModified();
	}

	void UserContentManager::LoadAll()
	{
		DEBUG_MEASURE_BEGIN("UserContentManager::LoadAll");

		auto& assetMngr = *_pAssetMngr;

		// Load characters
		for (auto& asset : _pAssetMngr->GetCharacterAssets())
		{
			if (asset.data_format == DataFormat::DataXml && asset.HasData())
			{
				CharacterData character;
				if (character.LoadFromXml(asset.AsStringView()) == FileError::NoError)
				{
					character.assetId = asset.id;
					character.createdAt = asset.GetCreatedAt();
					character.updatedAt = asset.GetUpdatedAt();
					_characters[asset.id] = std::move(character);
				}
			}
			else
				continue; // Skip
		}

		// Load scenarios
		for (auto& asset : _pAssetMngr->GetScenarioAssets())
		{
			if (asset.data_format == DataFormat::DataXml && asset.HasData())
			{
				ScenarioData scenario;
				if (scenario.LoadFromXml(asset.AsString()) == FileError::NoError)
					_scenarios[asset.id] = std::move(scenario);
			}
			else
				continue; // Skip
		}

		DEBUG_MEASURE_END();
	}

	std::optional<fig::io::CharacterData> UserContentManager::GetCharacter(const fig::uuid& id) const noexcept
	{
		if (auto itFind = _characters.find(id); itFind != _characters.cend())
			return std::make_optional(itFind->second);
		return std::nullopt;
	}

	std::optional<fig::io::ScenarioData> UserContentManager::GetScenario(const fig::uuid& id) const noexcept
	{
		if (auto itFind = _scenarios.find(id); itFind != _scenarios.cend())
			return std::make_optional(itFind->second);
		return std::nullopt;
	}

	std::optional<fig::llm::ModelSettings> UserContentManager::GetActiveModelSettings() const noexcept
	{
		fig::uuid activePresetId = Global::GetUserSettings().GetUUID(UserSetting::ModelPreset);
		std::optional<AssetRef> settingsAsset = std::nullopt;

		if (not activePresetId.empty())
			settingsAsset = _pAssetMngr->FindAsset(activePresetId, AssetType::ModelSettings);

		if (not settingsAsset.has_value())
		{
			// Find first
			auto model_settings = _pAssetMngr->GetAssetsOfType(AssetType::ModelSettings)
				| std::ranges::to<std::vector>();
			if (model_settings.size() > 0)
				settingsAsset = _pAssetMngr->FindAsset(model_settings.front().id);
		}

		if (settingsAsset.has_value())
		{
			if (auto try_load = _pAssetMngr->LoadAsset(settingsAsset.value().get().id))
			{
				auto& modelSettingsAsset = try_load.value().get();

				fig::llm::ModelSettings settings {};
				if (Success(settings.LoadFromXml(modelSettingsAsset.data)))
					return settings;
			}
		}
		return std::nullopt;
	}

	std::optional<std::reference_wrapper<fig::io::CardMetaData>> UserContentManager::GetMetaData(const fig::uuid& id, bool bIgnoreCache) noexcept
	{
		if (not bIgnoreCache)
		{
			if (auto itFind = _metaData.find(id); itFind != _metaData.cend())
				return std::make_optional(std::ref(itFind->second));
		}

		if (auto tryAsset = _pAssetMngr->FindAsset(id))
		{
			auto& asset = tryAsset.value().get();
			CardMetaData metaData = CardMetaData::FromJson(asset.settings).value_or({});
			metaData.createdAt = asset.GetCreatedAt();
			metaData.updatedAt = asset.GetUpdatedAt();
			metaData.lastUsedAt = asset.GetLastUsedAt();

			if (auto itFind = _characters.find(id); itFind != _characters.cend())
			{
				auto& character = itFind->second;
				metaData.name = character.shortName;
				metaData.gender = character.gender;
				metaData.chatCount = static_cast<uint32_t>(std::ranges::count_if(_pAssetMngr->GetAssets(), [&id](auto&& a) { return a.asset_type == AssetType::ChatInstance and a.parent_id == id; }));
			}
			_metaData[id] = metaData;
			return std::make_optional(std::ref(_metaData.at(id)));
		}

		return std::nullopt;
	}

	template <CardMetaData::Flag E>
	bool UserContentManager::MarkFlag(const fig::uuid& assetId, bool value)
	{
		if (auto tryAsset = _pAssetMngr->FindAsset(assetId))
		{
			auto& asset = tryAsset.value().get();
			if (auto tryMeta = GetMetaData(assetId))
			{
				auto& meta = tryMeta.value().get();
				value ? meta.flags.Set(E) : meta.flags.Unset(E);

				asset.SetSettings(CardMetaData::ToJson(meta));
				return true;
			}
		}
		return false;
	}

	bool UserContentManager::MarkImported(const fig::uuid& assetId, bool value)
	{
		return MarkFlag<CardMetaData::Flag::Imported>(assetId, value);
	}

	bool UserContentManager::MarkFavorite(const fig::uuid& assetId, bool value)
	{
		return MarkFlag<CardMetaData::Flag::Favorite>(assetId, value);
	}

	bool UserContentManager::MarkHidden(const fig::uuid& assetId, bool value)
	{
		return MarkFlag<CardMetaData::Flag::Hidden>(assetId, value);
	}

	bool UserContentManager::SetBorder(const fig::uuid& assetId, CardBorderStyle borderStyle)
	{
		if (auto tryAsset = _pAssetMngr->FindAsset(assetId))
		{
			auto& asset = tryAsset.value().get();
			if (auto tryMeta = GetMetaData(assetId))
			{
				auto& meta = tryMeta.value().get();
				meta.borderStyle = borderStyle;
				asset.SetSettings(CardMetaData::ToJson(meta));
				return true;
			}
		}
		return false;
	}

	size_t UserContentManager::ImportCharactersInDirectory(const fig::path& directory, size_t max_count)
	{
		auto imported = _pAssetMngr->ImportCharactersInDirectory(directory, AssetManager::CharacterDataFormat::TavernV2, max_count);
		for (auto& import : imported)
		{
			auto& asset = import.get();
			MarkImported(asset.id);
		}
		return imported.size();
	}

	std::expected<AssetRef, FileError> UserContentManager::ImportCharacter(const fig::path& filename)
	{
		if (auto imported = _pAssetMngr->ImportCharacter(filename, AssetManager::CharacterDataFormat::TavernV2))
		{
			auto& asset = imported.value().get();
			MarkImported(asset.id);
			return std::ref(asset);
		}
		else
			return std::unexpected(imported.error());
	}

	std::expected<AssetRef, FileError> UserContentManager::ImportScenario(const fig::path& filename)
	{
		if (auto imported = _pAssetMngr->ImportScenario(filename))
		{
			auto& asset = imported.value().get();
			MarkImported(asset.id);
			return std::ref(asset);
		}
		else
			return std::unexpected(imported.error());
	}

	AssetManager& UserContentManager::GetAssetManager()
	{
		return *_pAssetMngr;
	}

	void UserContentManager::SaveModified()
	{
		_pAssetMngr->SaveModified();
	}

	std::expected<fig::sdl::TextureRef, FileError> UserContentManager::GetSmallPortraitForCharacter(fig::gui::RendererPtr pRenderer, const fig::uuid& characterId) noexcept
	{
		if (auto find_asset = _pAssetMngr->FindAsset(characterId, ImageType::SmallPortrait))
		{
			auto& asset = find_asset.value().get();
			const fig::uuid& assetId = asset.id;
			int32_t width = asset.GetMeta<int32_t>(MetaTag::ImageWidth).value_or(Constants::Data::SmallPortraitWidth);
			int32_t height = asset.GetMeta<int32_t>(MetaTag::ImageHeight).value_or(Constants::Data::SmallPortraitWidth);

			if (auto itFind = _textures.find(assetId); itFind != _textures.cend())
				return std::ref(itFind->second);

			if (auto try_load = _pAssetMngr->LoadAsset(assetId))
			{
				auto& imageAsset = try_load.value().get();
				if (auto image = fig::gui::CreateSurfaceFromBytes(width, height, fig::gui::ImageFormat::RGB24, imageAsset.data); not image.empty())
				{
					auto pNewSurface = SDL_CreateSurface(Constants::Chat::SmallPortraitWidth, Constants::Chat::SmallPortraitWidth, SDL_PIXELFORMAT_RGBA8888);
					SDL_BlitSurfaceScaled(image.get(), NULL, pNewSurface, NULL, SDL_SCALEMODE_LINEAR);

					image.reset(pNewSurface);
					fig::gui::MaskCorners(image, fig::gui::MaskType::CARD_CORNER_MASK);

					auto& surface = _surfaces[assetId] = std::move(image);
					if (auto texture = fig::gui::CreateTexture(pRenderer, surface))
					{
						auto& result = _textures[assetId] = std::move(texture);
						return std::ref(result);
					}
				}
				return std::unexpected(FileError::UnrecognizedFormat);
			}
			else
			{
				return std::unexpected(try_load.error());
			}
		}
		return std::unexpected(FileError::NotFound);
	}
}