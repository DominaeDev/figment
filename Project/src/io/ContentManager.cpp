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

		// Instantiate caches
		_caches[AssetTypeOf<fig::data::Character>] = std::make_unique<AssetCache<fig::data::Character, AssetType::Character, DataFormat::DataXml>>(_pAssetMngr.get());
		_caches[AssetTypeOf<fig::data::Scenario>] = std::make_unique<AssetCache<fig::data::Scenario, AssetType::Scenario, DataFormat::DataXml>>(_pAssetMngr.get());
		_caches[AssetTypeOf<fig::data::ChatInstance>] = std::make_unique<AssetCache<fig::data::ChatInstance, AssetType::ChatInstance, DataFormat::DataXml>>(_pAssetMngr.get());
		_caches[AssetTypeOf<fig::data::ChatLog>] = std::make_unique<AssetCache<fig::data::ChatLog, AssetType::ChatLog, DataFormat::DataXml>>(_pAssetMngr.get());

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
					_characters[asset.id] = std::move(character);
				else
					assert(false && "Failed to load character");
			}
		}

		// Load scenarios
		for (auto& asset : _pAssetMngr->GetScenarioAssets())
		{
			if (asset.data_format == DataFormat::DataXml && asset.HasData())
			{
				Scenario scenario;
				if (scenario.LoadFromXml(asset.AsStringView()) == FileError::NoError)
					_scenarios[asset.id] = std::move(scenario);
			}
		}

		// Load chats
		for (auto& asset : _pAssetMngr->GetAssetsOfType(AssetType::ChatInstance))
		{
			if (asset.data_format == DataFormat::DataXml && asset.HasData())
			{
				ChatInstance chatInstance;
				if (chatInstance.LoadFromXml(asset.AsStringView()) == FileError::NoError)
					_chats[asset.id] = std::move(chatInstance);
			}
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
		auto& newAsset = _pAssetMngr->CreateAsset(AssetType::ChatInstance, DataFormat::DataXml, data);

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
		for (auto& kvp : _chats)
		{
			auto& assetId = kvp.first;
			auto& chat = kvp.second;

			for (auto& id : chat.characterIds)
			{
				if (not id.empty())
					_chatsByAsset[id].push_back(assetId);
			}

			if (not chat.userId.empty())
				_chatsByAsset[chat.userId].push_back(assetId);

			if (not chat.scenarioId.empty())
				_chatsByAsset[chat.scenarioId].push_back(assetId);

			if (not chat.worldId.empty())
				_chatsByAsset[chat.worldId].push_back(assetId);
		}
	}

	fig::cref_vector<Asset> UserContentManager::GetChatsWithCharacter(const fig::uuid& characterId, bool bLoad)
	{
		std::unordered_set<fig::uuid> instanceIds;
		for (auto& kvp : _chats)
		{
			if (kvp.second.contains(characterId))
				instanceIds.insert(kvp.first);
		}

		auto logAssets = _pAssetMngr->GetAssetsOfType(AssetType::ChatLog)
			| std::views::filter([&](auto&& a) { return instanceIds.contains(a.parent_id); })
			| std::views::transform([](auto&& a) { return std::cref(a); })
			| std::ranges::to<std::vector>();

		if (bLoad)
		{
			std::vector<fig::uuid> logIds = logAssets
				| std::views::transform([](auto&& a) { return a.get().id; })
				| std::ranges::to<std::vector>();
			_pAssetMngr->LoadDataAssets(logIds);
		}

		return logAssets;
	}

	fig::cref_vector<Asset> UserContentManager::GetChatLogs(bool bLoad)
	{
		auto chatInstanceIds = _chats
			| std::views::keys
			| std::ranges::to<std::unordered_set>();

		auto chatLogAssets = _pAssetMngr->GetAssetsOfType(AssetType::ChatLog)
			| std::views::filter([&](auto& a) { return chatInstanceIds.contains(a.parent_id); })
			| std::views::transform([](auto& a) { return std::cref(a); })
			| std::ranges::to<std::vector>();

		std::ranges::sort(chatLogAssets, std::ranges::greater(), [](auto& a) { return a.get().GetLastUsedAt(); });

		if (bLoad)
		{
			std::vector<fig::uuid> logIds = chatLogAssets
				| std::views::filter([](auto& a) { return !a.get().HasData(); })
				| std::views::transform([](auto& a) { return a.get().id; })
				| std::ranges::to<std::vector>();
			_pAssetMngr->LoadDataAssets(logIds);
		}

		return chatLogAssets;
	}

	std::optional<fig::string> UserContentManager::GetCharacterName(const fig::uuid& characterId) const
	{
		if (auto itFind = _characters.find(characterId); itFind != _characters.cend())
			return (*itFind).second.shortName;
		return std::nullopt;
	}

}