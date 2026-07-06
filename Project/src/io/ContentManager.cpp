#include <pch.h>
#include "app/AppState.h"
#include "io/ContentManager.h"
#include "io/AssetManager.h"
#include "gui/AppResources.h"
#include "data/ChatInstance.h"

using namespace fig::user;
using namespace fig::data;

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
				Character character;
				if (character.LoadFromXml(asset.AsStringView()) == FileError::NoError)
				{
//					character.createdAt = asset.GetCreatedAt();
//					character.updatedAt = asset.GetUpdatedAt();
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
				Scenario scenario;
				if (scenario.LoadFromXml(asset.AsString()) == FileError::NoError)
					_scenarios[asset.id] = std::move(scenario);
			}
			else
				continue; // Skip
		}

		RefreshChatCount();

		DEBUG_MEASURE_END();
	}

	fig::optional_cref<fig::data::Character> UserContentManager::GetCharacter(const fig::uuid& id) const noexcept
	{
		if (auto itFind = _characters.find(id); itFind != _characters.cend())
			return make_optional_cref(itFind->second);
		return fig::nullref;
	}

	fig::optional_cref<fig::data::Scenario> UserContentManager::GetScenario(const fig::uuid& id) const noexcept
	{
		if (auto itFind = _scenarios.find(id); itFind != _scenarios.cend())
			return make_optional_cref(itFind->second);
		return fig::nullref;
	}

	std::optional<ModelSettings> UserContentManager::GetActiveModelSettings() const noexcept
	{
		fig::uuid activePresetId = Global::GetUserSettings().GetUUID(UserSetting::ModelPreset);
		fig::optional_cref<Asset> settingsAsset;

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
			if (auto try_load = _pAssetMngr->LoadAsset((*settingsAsset).id))
			{
				auto& modelSettingsAsset = *try_load;

				fig::data::ModelSettings settings {};
				if (Success(settings.LoadFromXml(modelSettingsAsset.data)))
					return settings;
			}
		}
		return std::nullopt;
	}

	fig::optional_ref<fig::data::CardMetaData> UserContentManager::GetMetaData(const fig::uuid& id, bool bIgnoreCache) noexcept
	{
		if (not bIgnoreCache)
		{
			if (auto itFind = _metaData.find(id); itFind != _metaData.cend())
				return make_optional_ref(itFind->second);
		}

		if (auto tryAsset = _pAssetMngr->FindAsset(id))
		{
			auto& asset = tryAsset.value();
			CardMetaData metaData = CardMetaData::FromJson(asset.settings).value_or({});
			metaData.createdAt = asset.GetCreatedAt();
			metaData.updatedAt = asset.GetUpdatedAt();
			metaData.lastUsedAt = asset.GetLastUsedAt();

			if (auto itFind = _characters.find(id); itFind != _characters.cend())
			{
				auto& character = itFind->second;
				metaData.name = character.shortName;
				metaData.gender = character.gender;
				metaData.chatCount = static_cast<uint32_t>(GetChatCount(id));
			}
			_metaData[id] = metaData;
			return make_optional_ref(_metaData.at(id));
		}

		return fig::nullref;
	}

	template <CardMetaData::Flag E>
	bool UserContentManager::MarkFlag(const fig::uuid& assetId, bool value)
	{
		if (auto tryAsset = _pAssetMngr->FindAsset(assetId))
		{
			return _pAssetMngr->ModifyAsset(*tryAsset, [&](Asset& asset) -> bool {
				if (auto tryMeta = GetMetaData(assetId))
				{
					auto& meta = *tryMeta;
					value ? meta.flags.Set(E) : meta.flags.Unset(E);

					asset.SetSettings(CardMetaData::ToJson(meta));
					return true;
				}
				return false;
			});
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
			return _pAssetMngr->ModifyAsset(*tryAsset, [&](auto& asset) -> bool {
				if (auto tryMeta = GetMetaData(assetId))
				{
					auto& meta = *tryMeta;
					meta.borderStyle = borderStyle;
					asset.SetSettings(CardMetaData::ToJson(meta));
					return true;
				}
				return false;
			});
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

	fig::expected_ref<Asset, FileError> UserContentManager::ImportCharacter(const fig::path& filename)
	{
		if (auto imported = _pAssetMngr->ImportCharacter(filename, AssetManager::CharacterDataFormat::TavernV2))
		{
			auto& asset = *imported;
			MarkImported(asset.id);
			return asset;
		}
		else
			return unexpected(imported.error());
	}

	fig::expected_ref<Asset, FileError> UserContentManager::ImportScenario(const fig::path& filename)
	{
		if (auto imported = _pAssetMngr->ImportScenario(filename))
		{
			auto& asset = *imported;
			MarkImported(asset.id);
			return asset;
		}
		else
			return unexpected(imported.error());
	}

	AssetManager& UserContentManager::GetAssetManager()
	{
		return *_pAssetMngr;
	}

	void UserContentManager::SaveModified()
	{
		_pAssetMngr->SaveModified();
	}

	fig::expected_ref<fig::sdl::Texture, FileError> UserContentManager::GetSmallPortraitForCharacter(fig::gui::RendererPtr pRenderer, const fig::uuid& characterId) noexcept
	{
		if (auto find_asset = _pAssetMngr->FindAsset(characterId, ImageType::SmallPortrait))
		{
			auto& asset = *find_asset;
			const fig::uuid& assetId = asset.id;
			int32_t width = asset.GetMeta<int32_t>(MetaTag::ImageWidth).value_or(Constants::Data::SmallPortraitWidth);
			int32_t height = asset.GetMeta<int32_t>(MetaTag::ImageHeight).value_or(Constants::Data::SmallPortraitWidth);

			if (auto itFind = _textures.find(assetId); itFind != _textures.cend())
				return itFind->second;

			if (auto try_load = _pAssetMngr->LoadAsset(assetId))
			{
				auto& imageAsset = *try_load;
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
						return result;
					}
				}
				return unexpected(FileError::UnrecognizedFormat);
			}
			else
			{
				return unexpected(try_load.error());
			}
		}
		return unexpected(FileError::NotFound);
	}

	const Asset& UserContentManager::CreateAsset(const fig::data::ChatInstance& chatInstance)
	{
		fig::bytes data;
		chatInstance.SaveToXml(data);
		auto& newAsset = _pAssetMngr->CreateAsset(AssetType::ChatInstance, data);

		_pAssetMngr->ModifyAsset(newAsset, [&chatInstance](Asset& asset) {
			for (size_t idx = 0; idx < chatInstance.characterIds.size() && idx < fig::chat::MaxBots; ++idx)
			{
				auto& id = chatInstance.characterIds[idx];
				if (not id.empty())
					asset.SetMeta(static_cast<fig::io::MetaTag>(static_cast<uint8_t>(fig::io::MetaTag::ReferenceToCharacter) + static_cast<uint8_t>(idx)), id);
			}
			if (not chatInstance.userId.empty())
				asset.SetMeta(fig::io::MetaTag::ReferenceToUser, chatInstance.userId);
			if (not chatInstance.scenarioId.empty())
				asset.SetMeta(fig::io::MetaTag::ReferenceToScenario, chatInstance.scenarioId);
			if (not chatInstance.worldId.empty())
				asset.SetMeta(fig::io::MetaTag::ReferenceToWorld, chatInstance.worldId);
		});

		return newAsset;
	}

	size_t UserContentManager::GetChatCount(const fig::uuid& assetId)
	{
		if (auto itFind = _chatsByAsset.find(assetId); itFind != _chatsByAsset.cend())
			return itFind->second.size();
		return 0uz;
	}

	void UserContentManager::RefreshChatCount()
	{
		_chatsByAsset.clear();
		auto assets = _pAssetMngr->GetAssetsOfType(AssetType::ChatInstance);

		for (auto& asset : assets)
		{
			for (size_t i = 0; i < fig::chat::MaxBots; ++i)
			{
				if (auto characterId = asset.GetMeta<fig::uuid>(static_cast<MetaTag>(static_cast<uint8_t>(MetaTag::ReferenceToCharacter) + static_cast<uint8_t>(i))))
					_chatsByAsset[*characterId].push_back(asset.id);
			}

			if (auto userId = asset.GetMeta<fig::uuid>(MetaTag::ReferenceToUser))
				_chatsByAsset[*userId].push_back(asset.id);

			if (auto scenarioId = asset.GetMeta<fig::uuid>(MetaTag::ReferenceToScenario))
				_chatsByAsset[*scenarioId].push_back(asset.id);

			if (auto worldId = asset.GetMeta<fig::uuid>(MetaTag::ReferenceToWorld))
				_chatsByAsset[*worldId].push_back(asset.id);
		}
	}
}