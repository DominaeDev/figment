#include <pch.h>
#include "app/AppState.h"
#include "io/ContentManager.h"
#include "io/AssetManager.h"
#include "gui/AppResources.h"
#include "data/ChatInstance.h"
#include "data/VoiceSettings.h"

using namespace fig::user;
using namespace fig::data;
using namespace fig::gui;

namespace fig::io
{
	UserContentManager::UserContentManager(const fig::user::UserProfile& profile, const fig::auth::AuthKey& authKey)
	{
		_pAssetMngr = std::make_unique<AssetManager>(profile, authKey);

		// Instantiate caches
		_caches[AssetTypeOf<fig::data::Character>]		= std::make_unique<AssetCache<fig::data::Character, "Character">>(_pAssetMngr.get());
		_caches[AssetTypeOf<fig::data::Scenario>]		= std::make_unique<AssetCache<fig::data::Scenario, "Scenario">>(_pAssetMngr.get());
		_caches[AssetTypeOf<fig::data::ChatInstance>]	= std::make_unique<AssetCache<fig::data::ChatInstance, "ChatInstance">>(_pAssetMngr.get());
		_caches[AssetTypeOf<fig::data::ChatLog>]		= std::make_unique<AssetCache<fig::data::ChatLog, "ChatLog">>(_pAssetMngr.get());
		_caches[AssetTypeOf<fig::sdl::Surface>]			= std::make_unique<AssetCache<fig::sdl::Surface, "Image">>(_pAssetMngr.get());
		_caches[AssetTypeOf<fig::data::VoiceSettings>]	= std::make_unique<AssetCache<fig::data::VoiceSettings, "VoiceSettings">>(_pAssetMngr.get());

		Preload();
		RefreshChatCounts();
	}

	void UserContentManager::Preload()
	{
		DEBUG_MEASURE_BEGIN("UserContentManager::Preload");
		GetCache<fig::data::Character>().Preload();
		GetCache<fig::data::Scenario>().Preload();
		GetCache<fig::data::ChatInstance>().Preload();
		DEBUG_MEASURE_END();
	}

	std::optional<ModelSettings> UserContentManager::GetActiveModelSettings() const noexcept
	{
		fig::uuid activePresetId = Global::GetUserSettings().GetUUID(UserSetting::Settings::ModelPreset);
		fig::optional_cref<Asset> settingsAsset;

		if (not activePresetId.empty())
			settingsAsset = _pAssetMngr->FindAsset(activePresetId, AssetType::ModelSettings);

		if (not settingsAsset.has_value())
		{
			// Find first
			auto model_settings = _pAssetMngr->FindAssetsOfType(make_asset_type(AssetType::ModelSettings))
				| std::ranges::to<std::vector>();
			if (model_settings.size() > 0)
				settingsAsset = _pAssetMngr->FindAsset(model_settings.front().get().id);
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

	fig::optional_ref<ContentMetaData> UserContentManager::GetMetaData(const fig::uuid& assetId) noexcept
	{
		if (auto itFind = _metaData.find(assetId); itFind != _metaData.cend())
			return make_optional_ref(itFind->second);

		if (auto tryAsset = _pAssetMngr->FindAsset(assetId))
		{
			auto& asset = tryAsset.value();

			ContentMetaData meta;
			meta.assetType = asset.type;
			meta.parentId = asset.parent_id;
			meta.createdAt = asset.GetCreatedAt();
			meta.updatedAt = asset.GetUpdatedAt();
			meta.lastUsedAt = asset.GetUpdatedAt();

			if (asset.type.IsOfType(AssetType::Character))
			{
				if (auto try_character = Get<Character>(assetId))
				{
					meta.name = (*try_character).name.GetSpokenName();
					meta.gender = (*try_character).gender;
					meta.tags = (*try_character).GetTags();
				}

				// Last used => last chat
				if (auto lastChat = FindLastChatWith(asset.id))
					meta.lastUsedAt = std::max(meta.lastUsedAt, lastChat.value().GetUpdatedAt());

				meta.hasVoice = _pAssetMngr->FindAssetOfType(make_asset_type(AssetType::Audio, AudioAssetType::VoiceReference), assetId).has_value();
			}

			_metaData[assetId] = meta;
			return make_optional_ref(_metaData.at(assetId));
		}

		return fig::nullref;
	}

	AssetUserSettings UserContentManager::GetUserSettings(const fig::uuid& id) const noexcept
	{
		if (auto tryAsset = _pAssetMngr->FindAsset(id))
			return (*tryAsset).GetUserSettings().value_or({});
		return {};
	}

	template <AssetUserSettings::Flag E>
	bool UserContentManager::MarkFlag(const fig::uuid& assetId, bool value)
	{
		if (auto tryAsset = _pAssetMngr->FindAsset(assetId))
		{
			_pAssetMngr->ModifyAsset(*tryAsset, [&](Asset& asset) {
				auto& settings = asset.GetUserSettings();
				value ? settings.flags.Set(E) : settings.flags.Unset(E);
				asset.InvalidateUserSettings();
			});
			return true;
		}
		return false;
	}

	bool UserContentManager::MarkImported(const fig::uuid& assetId, bool value)
	{
		return MarkFlag<AssetUserSettings::Flag::Imported>(assetId, value);
	}

	bool UserContentManager::MarkFavorite(const fig::uuid& assetId, bool value)
	{
		return MarkFlag<AssetUserSettings::Flag::Favorite>(assetId, value);
	}

	bool UserContentManager::MarkHidden(const fig::uuid& assetId, bool value)
	{
		return MarkFlag<AssetUserSettings::Flag::Hidden>(assetId, value);
	}

	bool UserContentManager::SetBorder(const fig::uuid& assetId, CardBorderStyle borderStyle)
	{
		if (auto tryAsset = _pAssetMngr->FindAsset(assetId))
		{
			_pAssetMngr->ModifyAsset(*tryAsset, [&](auto& asset) {
				auto& settings = asset.GetUserSettings();
				settings.borderStyle = borderStyle;
				asset.InvalidateUserSettings();
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

	AssetManager& UserContentManager::GetAssets()
	{
		return *_pAssetMngr;
	}

	fig::observer_ptr<fig::sdl::Texture> UserContentManager::GetCachedTexture(fig::renderer_ptr pRenderer, const fig::uuid& assetId, fig::texture_ptr pMask)
	{
		if (auto itRenderer = _cachedTextures.find(pRenderer); itRenderer != _cachedTextures.cend())
		{
			if (auto itAsset = (*itRenderer).second.find(assetId); itAsset != (*itRenderer).second.cend())
			{
				if (auto itMask = (*itAsset).second.find(pMask); itMask != (*itAsset).second.cend())
					return &itMask->second;
			}
		}
		return nullptr;
	}

	fig::optional_cref<Asset> UserContentManager::GetLargePortraitForCharacter(const fig::uuid& characterId, size_t index) const
	{
		auto portraits = _pAssetMngr->FindAssetsOfType(make_asset_type(AssetType::Image, ImageAssetType::LargePortrait), characterId);
		if (not portraits.empty())
		{
			std::ranges::sort(portraits, std::ranges::less(), [](auto&& b) { return b.get().GetOrder(); });
			return portraits[std::min(index, portraits.size() - 1uz)].get();
		}
		return fig::nullref;
	}

	fig::expected_ref<fig::sdl::Texture, FileError> UserContentManager::GetSmallPortraitForCharacter(const fig::uuid& characterId, fig::texture_ptr pMask, fig::renderer_ptr pRenderer) noexcept
	{
		auto portraits = _pAssetMngr->FindAssetsOfType(make_asset_type(AssetType::Image, ImageAssetType::SmallPortrait), characterId);
		if (portraits.empty())
			return std::unexpected(FileError::NotFound);

		std::ranges::sort(portraits, std::ranges::less(), [](auto&& b) { return b.get().GetOrder(); });
		auto& portraitAsset = portraits[0].get();

		if (auto cached = GetCachedTexture(pRenderer, portraitAsset.id, pMask))
			return *cached;

		if (auto try_surface = Get<fig::sdl::Surface>(portraitAsset.id))
		{
			auto& surface = try_surface.value();

			if (auto pTexture = SDL_CreateTextureFromSurface(pRenderer, surface.get()))
			{
				fig::sdl::Texture texture = fig::sdl::Texture::from_ptr(pTexture);

				if (pMask)
				{
					auto priorRenderTarget = SDL_GetRenderTarget(pRenderer);

					// Bake mask into texture
					fig::texture_ptr pTarget = SDL_CreateTexture(pRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, pTexture->w, pTexture->h);
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
					SDL_SetRenderTarget(pRenderer, priorRenderTarget);
					SDL_SetTextureBlendMode(pTarget, SDL_BLENDMODE_BLEND_PREMULTIPLIED);
					texture.reset(pTarget);
				}

				auto& cache = _cachedTextures[pRenderer][portraitAsset.id];
				cache[pMask] = std::move(texture);
				return cache[pMask];
			}
		}
		
		return std::unexpected(FileError::ReadError);
	}

	fig::optional_cref<Asset> UserContentManager::GetBackgroundForCharacter(const fig::uuid& characterId, size_t index) const
	{
		auto backgrounds = _pAssetMngr->FindAssetsOfType(make_asset_type(AssetType::Image, ImageAssetType::Background), characterId);
		if (not backgrounds.empty())
		{
			std::ranges::sort(backgrounds, std::ranges::less(), [](auto&& b) { return b.get().GetOrder(); });
			return backgrounds[std::min(index, backgrounds.size() - 1uz)].get();
		}
		return fig::nullref;
	}

	fig::expected_cref<fig::sdl::Texture, FileError> UserContentManager::GetTexture(const fig::uuid& assetId, fig::renderer_ptr pRenderer) noexcept
	{
		if (auto cached = GetCachedTexture(pRenderer, assetId))
			return *cached;
		
		if (auto find_asset = _pAssetMngr->FindAsset(assetId, AssetType::Image))
		{
			auto& asset = *find_asset;
			if (auto try_surface = GetCache<fig::sdl::Surface>().Get(asset.id))
			{
				auto& surface = try_surface.value();

				if (auto pTexture = SDL_CreateTextureFromSurface(pRenderer, surface.get()))
				{
					fig::sdl::Texture texture = fig::sdl::Texture::from_ptr(pTexture);

					auto& cache = _cachedTextures[pRenderer][assetId];
					cache[nullptr] = std::move(texture);
					return cache[nullptr];
				}
			}
		}

		return unexpected(FileError::NotFound);
	}

	std::pair<fig::uuid, fig::uuid> UserContentManager::CreateChat(const fig::data::ChatInstance& chatInstance)
	{
		fig::bytes data;
		chatInstance.SaveToXml(data);
		auto& chatInstanceAsset = _pAssetMngr->CreateAsset(make_asset_type(AssetType::Chat, ChatAssetType::Instance, DataFormat::TextXml), data);

		_pAssetMngr->ModifyAsset(chatInstanceAsset, [&chatInstance](Asset& asset) {
			for (size_t idx = 0; idx < chatInstance.characterIds.size() && idx < 8uz; ++idx)
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

		auto& chatLogAsset = _pAssetMngr->CreateAsset(make_asset_type(AssetType::Chat, ChatAssetType::Log, DataFormat::TextXml), chatInstanceAsset.id);
		
		InvalidateChatCount(chatInstanceAsset.id);
		GetCache<ChatInstance>().Insert(chatInstanceAsset.id, chatInstance);

		return { chatInstanceAsset.id, chatLogAsset.id };
	}

	void UserContentManager::InvalidateCache(const fig::uuid& assetId) noexcept
	{
		for (auto& kvp : _caches)
		{
			if (kvp.second->Erase(assetId))
				return;
		}

		for (auto& cache : _cachedTextures)
		{
			auto& map = cache.second;
			for (auto& kvp : map)
				map.erase(assetId);
		}
	}

	void UserContentManager::RefreshChatCounts()
	{
		_chatCounts.clear();

		auto fnInc = [&](const fig::uuid& assetId) {
			if (auto it = _chatCounts.find(assetId); it != _chatCounts.end())
				++(it->second);
			else
				_chatCounts[assetId] = 1uz;
		};

		// Zero counts
		auto characterAssets = _pAssetMngr->FindAssetsOfType(make_asset_type(AssetType::Character));
		for (auto& assetRef : characterAssets)
			_chatCounts[assetRef.get().id] = 0uz;

		auto chatAssets = _pAssetMngr->FindAssetsOfType(make_asset_type(AssetType::Chat, ChatAssetType::Instance));
		for (auto assetRef : chatAssets)
		{
			auto& asset = assetRef.get();
			if (auto userSettings = asset.GetUserSettings(); userSettings.has_value() and (*userSettings).HasFlag(AssetUserSettings::Flag::Hidden))
				continue; // Don't count archived chats

			for (uint8_t idx = static_cast<uint8_t>(MetaTag::ReferenceToCharacter); idx < static_cast<uint8_t>(MetaTag::ReferenceToUser); ++idx)
			{
				if (auto refId = asset.GetMeta<fig::uuid>(static_cast<MetaTag>(idx)))
					fnInc(*refId);
			}
			if (auto refId = asset.GetMeta<fig::uuid>(MetaTag::ReferenceToUser))
				fnInc(*refId);
			if (auto refId = asset.GetMeta<fig::uuid>(MetaTag::ReferenceToScenario))
				fnInc(*refId);
			if (auto refId = asset.GetMeta<fig::uuid>(MetaTag::ReferenceToWorld))
				fnInc(*refId);
		}

		_chatCounts.erase(fig::uuid {});
	}

	size_t UserContentManager::GetChatCount(const fig::uuid& assetId)
	{
		if (auto itFind = _chatCounts.find(assetId); itFind != _chatCounts.cend())
			return itFind->second;

		auto chatAssets = _pAssetMngr->FindAssetsOfType(make_asset_type(AssetType::Chat, ChatAssetType::Instance));
		size_t count = 0uz;

		for (auto assetRef : chatAssets)
		{
			auto& chat = assetRef.get();
			if (auto userSettings = chat.GetUserSettings(); userSettings.has_value() and (*userSettings).HasFlag(AssetUserSettings::Flag::Hidden))
				continue; // Don't count archived chats

			if (chat.HasReferenceTo(assetId))
				++count;
		}
		
		_chatCounts[assetId] = count;
		return count;
	}

	fig::cref_vector<Asset> UserContentManager::GetChatInstances(bool bLoad) const noexcept
	{
		auto chatInstanceAssets = _pAssetMngr->FindAssetsOfType(make_asset_type(AssetType::Chat, ChatAssetType::Instance))
			| std::ranges::to<std::vector>();

		auto chatInstanceIds = chatInstanceAssets
			| std::views::transform([](auto&& a) { return a.get().id; })
			| std::ranges::to<std::unordered_set>();

		auto chatLogAssets = _pAssetMngr->FindAssetsOfType(make_asset_type(AssetType::Chat, ChatAssetType::Log))
			| std::views::filter([&](auto&& a) { return chatInstanceIds.contains(a.get().parent_id); })
			| std::ranges::to<std::vector>();

		if (bLoad)
		{
			// Load instances
			std::vector<fig::uuid> instanceIds = chatInstanceAssets
				| std::views::filter([](auto& a) { return !a.get().HasData(); })
				| std::views::transform([](auto& a) { return a.get().id; })
				| std::ranges::to<std::vector>();
			_pAssetMngr->LoadAssetData(instanceIds);

			// Load logs
			std::vector<fig::uuid> logIds = chatLogAssets
				| std::views::filter([](auto& a) { return !a.get().HasData(); })
				| std::views::transform([](auto& a) { return a.get().id; })
				| std::ranges::to<std::vector>();
			_pAssetMngr->LoadAssetData(logIds);
		}

		return chatInstanceAssets;
	}

	ChatCollection UserContentManager::GetAllChats() noexcept
	{
		auto chatInstanceAssets = _pAssetMngr->FindAssetsOfType(make_asset_type(AssetType::Chat, ChatAssetType::Instance))
			| std::ranges::to<std::vector>();

		return CompileChatCollection(chatInstanceAssets);
	}

	ChatCollection UserContentManager::GetChatsWith(const fig::uuid& characterId) noexcept
	{
		auto chatInstanceAssets = _pAssetMngr->FindAssetsOfType(make_asset_type(AssetType::Chat, ChatAssetType::Instance))
			| std::views::filter([&characterId](auto&& a) { return a.get().HasReferenceTo(characterId); })
			| std::ranges::to<std::vector>();

		return CompileChatCollection(chatInstanceAssets);
	}

	ChatCollection UserContentManager::CompileChatCollection(const fig::cref_vector<Asset>& assets) noexcept
	{
		auto chatInstanceIds = assets
			| std::views::transform([](auto&& a) { return a.get().id; })
			| std::ranges::to<std::unordered_set>();

		auto chatLogAssets = _pAssetMngr->FindAssetsOfType(make_asset_type(AssetType::Chat, ChatAssetType::Log))
			| std::views::filter([&](auto&& a) { return chatInstanceIds.contains(a.get().parent_id); })
			| std::ranges::to<std::vector>();

		ChatCollection chats;
		for (auto chatInstanceAssetRef : assets)
		{
			auto& chatInstanceAsset = chatInstanceAssetRef.get();
			if (auto try_instance = Get<ChatInstance>(chatInstanceAsset.id))
			{
				std::vector<UserContent<ChatLog>> logs;

				for (auto& chatLogAssetRef : chatLogAssets)
				{
					auto& chatLogAsset = chatLogAssetRef.get();
					if (chatLogAsset.parent_id != chatInstanceAsset.id)
						continue;

					if (auto try_log = Get<ChatLog>(chatLogAsset.id))
					{
						logs.push_back(UserContent<ChatLog> {
							.instance = std::cref(try_log.value()),
							.assetId = chatLogAsset.id,
							.createdAt = chatLogAsset.GetCreatedAt(),
							.updatedAt = chatLogAsset.GetUpdatedAt(),
						});
					}
				}

				if (not logs.empty())
				{
					std::ranges::sort(logs, std::ranges::greater(), [](auto& log) { return log.updatedAt; });

					chats.push_back({ 
						UserContent<ChatInstance> {
							.instance = std::cref(try_instance.value()),
							.assetId = chatInstanceAsset.id,
							.createdAt = chatInstanceAsset.GetCreatedAt(),
							.updatedAt = chatInstanceAsset.GetUpdatedAt(),
						}, 
						std::move(logs)
					});
				}
			}
		}
		return chats;
	}

	std::optional<fig::string> UserContentManager::GetCharacterName(const fig::uuid& characterId) const
	{
		if (auto try_character = GetCache<Character>().TryGet(characterId))
			return (*try_character).name.GetSpokenName();
		return std::nullopt;
	}

	fig::optional_cref<Asset> UserContentManager::FindLastChatWith(const fig::uuid& characterId) const
	{
		auto chatInstanceIds = _pAssetMngr->FindAssetsOfType(make_asset_type(AssetType::Chat, ChatAssetType::Instance))
			| std::views::filter([&](auto& a) { return a.get().HasReferenceTo(characterId); })
			| std::views::transform([](auto& a) { return a.get().id; })
			| std::ranges::to<std::unordered_set>();

		auto chatLogs = _pAssetMngr->FindAssetsOfType(make_asset_type(AssetType::Chat, ChatAssetType::Log))
			| std::views::filter([&](auto& a) { return chatInstanceIds.contains(a.get().parent_id); })
			| std::views::transform([](auto& a) { return std::cref(a); })
			| std::ranges::to<std::vector>();

		if (not chatLogs.empty())
		{
			std::ranges::sort(chatLogs, std::ranges::greater(), [](auto& a) { return a.get().GetUpdatedAt(); });
			return chatLogs[0].get();
		}
		return nullref;
	}

	bool UserContentManager::DeleteAsset(fig::uuid assetId)
	{
		// Invalidate associated meta data
		auto associatedAssets = _pAssetMngr->FindAssociatedAssets(assetId);
		for (auto& id : associatedAssets)
			InvalidateMeta(id);
		InvalidateAsset(assetId);

		if (not _pAssetMngr->DeleteAsset(assetId))
			return false;
		return true;
	}

	size_t UserContentManager::DeleteAssets(std::span<fig::uuid> assetIds)
	{
		// Invalidate associated meta data
		for (auto& assetId : assetIds)
		{
			auto associatedAssets = _pAssetMngr->FindAssociatedAssets(assetId);
			for (auto& id : associatedAssets)
				InvalidateMeta(id);
			InvalidateAsset(assetId);
		}

		return _pAssetMngr->DeleteAssets(assetIds);
	}

	fig::cref_vector<Asset> UserContentManager::GetCharacters() const noexcept
	{ 
		return _pAssetMngr->FindAssetsOfType(make_asset_type(AssetType::Character));
	}

	fig::cref_vector<Asset> UserContentManager::GetScenarios() const noexcept
	{ 
		return _pAssetMngr->FindAssetsOfType(make_asset_type(AssetType::Scenario));
	}

	fig::uuid UserContentManager::CreateVoiceReference(const fig::uuid& characterId, const fig::data::VoiceSettings& voiceSettings)
	{
		if (auto character = _pAssetMngr->FindAsset(characterId, AssetType::Character); not character.has_value())
			return {}; // Not found

		if (voiceSettings.voicePrint.audioData.empty())
			return {}; // No data

		// Delete existing voice
		if (auto existingVoice = _pAssetMngr->FindAssetOfType(make_asset_type(AssetType::Audio, AudioAssetType::VoiceSettings), characterId))
			DeleteAsset(existingVoice.value().id);

		fig::bytes voiceSettingsData;
		voiceSettings.SaveToXml(voiceSettingsData);
		auto& voiceSettingsAsset = _pAssetMngr->CreateAsset(make_asset_type(AssetType::Audio, AudioAssetType::VoiceSettings, DataFormat::TextXml), voiceSettingsData, characterId);

		return voiceSettingsAsset.id;
	}

	fig::optional_cref<VoiceSettings> UserContentManager::GetVoiceForCharacter(const fig::uuid& characterId) noexcept
	{
		if (auto try_asset = _pAssetMngr->FindAssetOfType(make_asset_type(AssetType::Audio, AudioAssetType::VoiceSettings), characterId))
		{
			return Get<fig::data::VoiceSettings>((*try_asset).id);
		}
		return std::nullopt;
	}

	bool UserContentManager::UpdateAsset(const fig::uuid& assetId, fig::bytes&& data)
	{
		if (_pAssetMngr->UpdateAsset(assetId, data))
		{
			InvalidateAsset(assetId);
			return true;
		}
		return false;
	}

	fig::optional_cref<Asset> UserContentManager::ReplaceCoverImage(const fig::uuid& characterId, const fig::uuid& originalAssetId)
	{
		if (auto originalAsset = _pAssetMngr->FindAsset(originalAssetId, AssetType::Image))
		{
			// Find prior cover asset
			fig::uuid previousCoverId;
			if (auto try_cover = _pAssetMngr->FindAssetOfType(make_asset_type(AssetType::Image, ImageAssetType::CoverImage), characterId))
				previousCoverId = (*try_cover).id;

			if (auto try_surface = Get<fig::sdl::Surface>(originalAssetId))
			{
				if (auto cover = fig::CreateCoverImage(*try_surface, false); not cover.empty())
				{
					// Create new cover asset
					auto& coverAsset = _pAssetMngr->CreateImageAsset(ImageAssetType::CoverImage, cover, characterId);

					_pAssetMngr->ModifyAsset(coverAsset, [&originalAssetId](auto&& asset) {
						asset.SetMeta(MetaTag::ReferenceToOriginal, originalAssetId);
					});

					// Delete previous cover asset
					if (not previousCoverId.empty())
						DeleteAsset(previousCoverId);
					
					return coverAsset;
				}
			}
		}

		// Not found
		return std::nullopt;
	}

	fig::optional_cref<Asset> UserContentManager::ReplaceSmallPortrait(const fig::uuid& characterId, const fig::sdl::Surface& image, const fig::uuid& originalAssetId)
	{
		// Find prior small portrait
		fig::uuid previousSmallPortrait;
		if (auto try_cover = _pAssetMngr->FindAssetOfType(make_asset_type(AssetType::Image, ImageAssetType::SmallPortrait), characterId))
			previousSmallPortrait = (*try_cover).id;

		if (not image.empty())
		{
			// Create new small portrait asset
			auto& smallPortraitAsset = _pAssetMngr->CreateImageAsset(ImageAssetType::SmallPortrait, image, characterId);

			if (not originalAssetId.empty())
			{
				_pAssetMngr->ModifyAsset(smallPortraitAsset, [&originalAssetId](auto&& asset) {
					asset.SetMeta(MetaTag::ReferenceToOriginal, originalAssetId);
				});
			}

			// Delete previous cover asset
			if (not previousSmallPortrait.empty())
				DeleteAsset(previousSmallPortrait);
			return smallPortraitAsset;
		}
		return std::nullopt;
	}

	void UserContentManager::AssignOrder(const std::vector<fig::uuid>& assetIds)
	{
		_pAssetMngr->ModifyAssets(assetIds, [](const fig::ref_vector<Asset>& assets) {
			int32_t i = 0;
			for (auto& asset : assets)
			{
				asset.get().GetUserSettings().order = i++;
				asset.get().InvalidateUserSettings();
			}
		});
	}

	std::expected<fig::timestamp, FileError> UserContentManager::GetCreatedAt(const fig::uuid& assetId) const noexcept
	{
		if (auto try_asset = _pAssetMngr->FindAsset(assetId))
			return (*try_asset).GetCreatedAt();
		return std::unexpected(FileError::NotFound);
	}

	std::expected<fig::timestamp, FileError> UserContentManager::GetUpdatedAt(const fig::uuid& assetId) const noexcept
	{
		if (auto try_asset = _pAssetMngr->FindAsset(assetId))
			return (*try_asset).GetUpdatedAt();
		return std::unexpected(FileError::NotFound);
	}

	void UserContentManager::InvalidateChatCount(const fig::uuid& assetId)
	{
		auto associatedAssets = _pAssetMngr->FindAssociatedAssets(assetId);
		for (auto& id : associatedAssets)
			_chatCounts.erase(id);
	}
}