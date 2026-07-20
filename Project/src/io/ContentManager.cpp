#include <pch.h>
#include "app/AppState.h"
#include "io/ContentManager.h"
#include "io/AssetManager.h"
#include "gui/AppResources.h"
#include "data/ChatInstance.h"

using namespace fig::user;
using namespace fig::data;
using namespace fig::gui;

namespace fig::io
{
	UserContentManager::UserContentManager(const fig::user::UserProfile& profile, const fig::auth::AuthKey& authKey)
	{
		_pAssetMngr = std::make_unique<AssetManager>(profile, authKey);

		// Instantiate caches
		_caches[AssetTypeOf<fig::data::Character>] = std::make_unique<AssetCache<fig::data::Character, AssetType::Character, DataFormat::DataXml, "Character">>(_pAssetMngr.get());
		_caches[AssetTypeOf<fig::data::Scenario>] = std::make_unique<AssetCache<fig::data::Scenario, AssetType::Scenario, DataFormat::DataXml, "Scenario">>(_pAssetMngr.get());
		_caches[AssetTypeOf<fig::data::ChatInstance>] = std::make_unique<AssetCache<fig::data::ChatInstance, AssetType::ChatInstance, DataFormat::DataXml, "ChatInstance">>(_pAssetMngr.get());
		_caches[AssetTypeOf<fig::data::ChatLog>] = std::make_unique<AssetCache<fig::data::ChatLog, AssetType::ChatLog, DataFormat::DataXml, "ChatLog">>(_pAssetMngr.get());
		_caches[AssetTypeOf<fig::sdl::Surface>] = std::make_unique<AssetCache<fig::sdl::Surface, AssetType::Image, DataFormat::Undefined, "Image">>(_pAssetMngr.get());

		LoadAll();
	}

	UserContentManager::~UserContentManager()
	{
		_pAssetMngr->SaveModified();
	}

	void UserContentManager::LoadAll()
	{
		DEBUG_MEASURE_BEGIN("UserContentManager::LoadAll");

		GetCache<fig::data::Character>().Preload();
		GetCache<fig::data::Scenario>().Preload();
		GetCache<fig::data::ChatInstance>().Preload();

		RefreshChatCount();

		DEBUG_MEASURE_END();
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

	fig::optional_ref<ContentMetaData> UserContentManager::GetMetaData(const fig::uuid& id) noexcept
	{
		if (auto itFind = _metaData.find(id); itFind != _metaData.cend())
			return make_optional_ref(itFind->second);

		if (auto tryAsset = _pAssetMngr->FindAsset(id))
		{
			auto& asset = tryAsset.value();

			ContentMetaData metaData;
			metaData.createdAt = asset.GetCreatedAt();
			metaData.updatedAt = asset.GetUpdatedAt();
			metaData.lastUsedAt = asset.GetLastUsedAt();

			if (asset.asset_type == AssetType::Character)
			{
				// Count chats
				metaData.chatCount = static_cast<uint32_t>(GetChatCount(id));

				// Last used => last chat
				if (auto lastChat = FindLastChatWith(asset.id))
					metaData.lastUsedAt = std::max(metaData.lastUsedAt, lastChat.value().GetLastUsedAt());
			}

			_metaData[id] = metaData;
			return make_optional_ref(_metaData.at(id));
		}

		return fig::nullref;
	}

	ContentUserSettings UserContentManager::GetUserSettings(const fig::uuid& id) noexcept
	{
		if (auto itFind = _userSettings.find(id); itFind != _userSettings.cend())
			return itFind->second;

		if (auto tryAsset = _pAssetMngr->FindAsset(id))
		{
			auto& asset = tryAsset.value();
			return _userSettings[id] = asset.GetUserSettings();
		}
		return {};
	}

	template <ContentUserSettings::Flag E>
	bool UserContentManager::MarkFlag(const fig::uuid& assetId, bool value)
	{
		if (auto tryAsset = _pAssetMngr->FindAsset(assetId))
		{
			_pAssetMngr->ModifyAsset(*tryAsset, [&](Asset& asset) {
				auto settings = GetUserSettings(assetId);
				value ? settings.flags.Set(E) : settings.flags.Unset(E);
				asset.SetUserSettings(settings);
			});
			InvalidateUserSettings((*tryAsset).id);
			return true;
		}
		return false;
	}

	bool UserContentManager::MarkImported(const fig::uuid& assetId, bool value)
	{
		return MarkFlag<ContentUserSettings::Flag::Imported>(assetId, value);
	}

	bool UserContentManager::MarkFavorite(const fig::uuid& assetId, bool value)
	{
		return MarkFlag<ContentUserSettings::Flag::Favorite>(assetId, value);
	}

	bool UserContentManager::MarkHidden(const fig::uuid& assetId, bool value)
	{
		return MarkFlag<ContentUserSettings::Flag::Hidden>(assetId, value);
	}

	bool UserContentManager::SetBorder(const fig::uuid& assetId, CardBorderStyle borderStyle)
	{
		if (auto tryAsset = _pAssetMngr->FindAsset(assetId))
		{
			_pAssetMngr->ModifyAsset(*tryAsset, [&](auto& asset) {
				auto settings = asset.GetUserSettings();
				settings.borderStyle = borderStyle;
				asset.SetUserSettings(settings);
			});
			InvalidateUserSettings((*tryAsset).id);
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

	fig::expected_ref<fig::sdl::Texture, FileError> UserContentManager::GetSmallPortraitForCharacter(const fig::uuid& characterId, TexturePtr pMask, RendererPtr pRenderer) noexcept
	{
		if (auto itRenderer = _cachedTextures.find(pRenderer); itRenderer != _cachedTextures.cend())
		{
			if (auto itTextures = (*itRenderer).second.find(characterId); itTextures != (*itRenderer).second.cend())
			{
				for (auto& t : (*itTextures).second)
				{
					if (t.pMask == pMask)
						return t.pTexture;
				}
			}
		}

		if (auto find_asset = _pAssetMngr->FindImageAsset(characterId, ImageType::SmallPortrait))
		{
			auto& asset = *find_asset;
			if (auto try_surface = GetCache<fig::sdl::Surface>().Get(asset.id))
			{
				auto& surface = try_surface.value();

				if (auto pTexture = SDL_CreateTextureFromSurface(pRenderer, surface.get()))
				{
					fig::sdl::Texture texture = fig::sdl::Texture::from_ptr(pTexture);

					if (pMask)
					{
						// Bake mask into texture
						TexturePtr pTarget = SDL_CreateTexture(pRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, pTexture->w, pTexture->h);
						SDL_SetRenderTarget(pRenderer, pTarget);
						SDL_SetRenderDrawColor(pRenderer, 0, 0, 0, 0);
						SDL_RenderClear(pRenderer);
						SDL_SetTextureBlendMode(pMask, SDL_BLENDMODE_NONE);
						SDL_RenderTexture(pRenderer, pMask, NULL, NULL);

						SDL_BlendMode multiplyAlpha = SDL_ComposeCustomBlendMode(
							SDL_BLENDFACTOR_DST_ALPHA,
							SDL_BLENDFACTOR_ZERO,
							SDL_BLENDOPERATION_ADD,
							SDL_BLENDFACTOR_ZERO,
							SDL_BLENDFACTOR_ONE,
							SDL_BLENDOPERATION_ADD
						);

						SDL_SetTextureBlendMode(pTexture, multiplyAlpha);
						SDL_RenderTexture(pRenderer, pTexture, NULL, NULL);
						SDL_SetRenderTarget(pRenderer, NULL);
						SDL_SetTextureBlendMode(pTarget, SDL_BLENDMODE_BLEND_PREMULTIPLIED);
						texture.reset(pTarget);
					}

					auto& textures = _cachedTextures[pRenderer][characterId];
					textures.emplace_back(CachedTexture {
						.pTexture = std::move(texture),
						.pMask = pMask,
					});

					return textures.back().pTexture;
				}
			}
		}

		return unexpected(FileError::NotFound);
	}

	fig::expected_cref<fig::sdl::Texture, FileError> UserContentManager::GetTexture(const fig::uuid& assetId, RendererPtr pRenderer) noexcept
	{
		if (auto itRenderer = _cachedTextures.find(pRenderer); itRenderer != _cachedTextures.cend())
		{
			if (auto itTextures = (*itRenderer).second.find(assetId); itTextures != (*itRenderer).second.cend())
			{
				for (auto& t : (*itTextures).second)
				{
					if (t.pMask == nullptr)
						return t.pTexture;
				}
			}
		}

		if (auto find_asset = _pAssetMngr->FindAsset(assetId, AssetType::Image))
		{
			auto& asset = *find_asset;
			if (auto try_surface = GetCache<fig::sdl::Surface>().Get(asset.id))
			{
				auto& surface = try_surface.value();

				if (auto pTexture = SDL_CreateTextureFromSurface(pRenderer, surface.get()))
				{
					fig::sdl::Texture texture = fig::sdl::Texture::from_ptr(pTexture);

					auto& textures = _cachedTextures[pRenderer][assetId];
					auto& value = textures.emplace_back(CachedTexture {
						.pTexture = std::move(texture),
					});

					return value.pTexture;
				}
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
		auto& chats = GetCache<ChatInstance>().GetAll();

		for (auto& kvp : chats)
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

	fig::cref_vector<Asset> UserContentManager::GetChatLogs(bool bLoad)
	{
		auto& chats = GetCache<ChatInstance>().GetAll();

		auto chatInstanceIds = chats
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
			_pAssetMngr->LoadAssetData(logIds);
		}

		return chatLogAssets;
	}

	fig::cref_vector<Asset> UserContentManager::GetChatLogsWith(const fig::uuid& characterId, bool bLoad)
	{
		std::unordered_set<fig::uuid> instanceIds;
		auto& chats = GetCache<ChatInstance>().GetAll();
		for (auto& kvp : chats)
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
			_pAssetMngr->LoadAssetData(logIds);
		}

		return logAssets;
	}
	std::optional<fig::string> UserContentManager::GetCharacterName(const fig::uuid& characterId) const
	{
		if (auto try_character = GetCache<Character>().TryGet(characterId))
			return (*try_character).shortName;
		return std::nullopt;
	}

	fig::optional_cref<Asset> UserContentManager::FindLastChatWith(const fig::uuid& characterId) const
	{
		auto chatInstanceIds = _pAssetMngr->GetAssetsOfType(AssetType::ChatInstance)
			| std::views::filter([&](auto& a) { return a.HasReferenceTo(characterId); })
			| std::views::transform([](auto& a) { return a.id; })
			| std::ranges::to<std::unordered_set>();

		auto chatLogs = _pAssetMngr->GetAssetsOfType(AssetType::ChatLog)
			| std::views::filter([&](auto& a) { return chatInstanceIds.contains(a.parent_id); })
			| std::views::transform([](auto& a) { return std::cref(a); })
			| std::ranges::to<std::vector>();

		if (not chatLogs.empty())
		{
			std::ranges::sort(chatLogs, std::ranges::greater(), [](auto& a) { return a.get().GetLastUsedAt(); });
			return chatLogs[0].get();
		}
		return nullref;
	}
}