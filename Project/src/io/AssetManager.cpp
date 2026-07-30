#include <pch.h>
#include <execution>
#include "io/AssetManager.h"
#include "user/UserManager.h"
#include "app/AppState.h"
#include "data/Character.h"
#include "data/Scenario.h"
#include "user/UserProfile.h"
#include "io/Xml.h"
#include "io/AssetFileWriter.h"
#include "io/AssetFileReader.h"
#include "io/FileUtility.h"
#include "io/CardImporter.h"
#include "gui/AppResources.h"
#include <cassert>

using namespace fig::gui;
using namespace fig::data;

namespace fig::io
{
	constexpr auto AutosaveInterval = std::chrono::seconds(120);

	AssetManager::AssetManager(const fig::user::UserProfile& profile, const fig::auth::AuthKey& authKey, int32_t worker_threads)
	{
		_profileAuthKey = authKey;
		_profileID = profile.id;
		_profilePath = profile.GetPath();

		LoadIndexDatabase();

		_workers.reserve(worker_threads);
		for (int i = 0; i < worker_threads; ++i)
			_workers.emplace_back([this](std::stop_token stop) { __Worker(stop); });

		_autosave_worker = std::jthread(std::bind_front(&AssetManager::__Autosave, this), AutosaveInterval);
	}

	AssetManager::~AssetManager()
	{
		Shutdown();
	}

	void AssetManager::Shutdown()
	{
		// Cancel all worker threads
		{	std::scoped_lock lock(_pending_mutex);
			while (!_pending.empty())
			{
				PendingRequest request = std::move(const_cast<PendingRequest&>(_pending.top()));
				_pending.pop();
				request.promise->set_value(std::unexpected(AsyncLoadError::Canceled));
			}
		}

		for (auto& worker : _workers)
			worker.request_stop();
		_pending_cv.notify_all();
		_workers.clear();

		// Stop auto save
		_autosave_worker.request_stop();
		_autosave_cv.notify_all();
		_autosave_worker = {}; // join

		SaveModifiedAssets();
	}

	void AssetManager::SaveNow()
	{
		SaveModifiedAssets();
	}

	const Asset& AssetManager::CreateAsset(AssetType type, const fig::uuid& parent) noexcept
	{
		std::scoped_lock lock { _assetsMutex };

		return CreateAsset_NoLock(make_asset_type(type), parent);
	}

	const Asset& AssetManager::CreateAsset(AssetTypeDefinition type, const fig::uuid& parent) noexcept
	{
		std::scoped_lock lock { _assetsMutex };

		return CreateAsset_NoLock(type, parent);
	}

	const Asset& AssetManager::CreateAsset(AssetType type, fig::bytes&& data, const fig::uuid& parent, bool bChecksum) noexcept
	{
		std::scoped_lock lock { _assetsMutex };

		return CreateAsset_NoLock(make_asset_type(type), std::move(data), parent, bChecksum);
	}

	const Asset& AssetManager::CreateAsset(AssetType type, fig::byte_span data, const fig::uuid& parent, bool bChecksum) noexcept
	{
		std::scoped_lock lock { _assetsMutex };

		return CreateAsset_NoLock(make_asset_type(type), data, parent, bChecksum);
	}

	const Asset& AssetManager::CreateAsset(AssetTypeDefinition type, fig::bytes&& data, const fig::uuid& parent, bool bChecksum) noexcept
	{
		std::scoped_lock lock { _assetsMutex };

		return CreateAsset_NoLock(type, std::move(data), parent, bChecksum);
	}

	const Asset& AssetManager::CreateAsset(AssetTypeDefinition type, fig::byte_span data, const fig::uuid& parent, bool bChecksum) noexcept
	{
		std::scoped_lock lock { _assetsMutex };

		return CreateAsset_NoLock(type, data, parent, bChecksum);
	}

	const Asset& AssetManager::CreateImageAsset(ImageAssetType subtype, const fig::sdl::Surface& surface, const fig::uuid& parent) noexcept
	{
		std::scoped_lock lock { _assetsMutex };
		return CreateImageAsset_NoLock(subtype, surface, parent);
	}

	Asset& AssetManager::CreateAsset_NoLock(AssetTypeDefinition type, const fig::uuid& parent) noexcept
	{
		fig::uuid id = GenerateUUID();
		auto& asset = _assets[id] = Asset {};
		asset.id = id;
		asset.parent_id = parent;
		asset.type = type;
		asset.sync_state.file_sync = AssetSyncState::Status::Created;
		asset.sync_state.db_sync = AssetSyncState::Status::Created;
		asset.sync_state.has_meta = true;

		auto now = fig::now();
		asset.SetMeta(MetaTag::CreatedAt, now);
		asset.SetMeta(MetaTag::UpdatedAt, now);
		return asset;
	}

	Asset& AssetManager::CreateAsset_NoLock(AssetTypeDefinition type, fig::bytes&& data, const fig::uuid& parent, bool bChecksum) noexcept
	{
		fig::uuid id = GenerateUUID();
		auto& asset = _assets[id] = Asset {};
		asset.id = id;
		asset.parent_id = parent;
		asset.type = type;
		asset.data = std::move(data); // Move data
		asset.sync_state.file_sync = AssetSyncState::Status::Created;
		asset.sync_state.db_sync = AssetSyncState::Status::Created;
		asset.sync_state.has_meta = true;
		asset.sync_state.has_data = not asset.data.empty();
		auto now = fig::now();
		asset.SetMeta(MetaTag::CreatedAt, now);
		asset.SetMeta(MetaTag::UpdatedAt, now);

		if (bChecksum)
			asset.CalculateChecksum();
		return asset;
	}

	Asset& AssetManager::CreateAsset_NoLock(AssetTypeDefinition type, fig::byte_span data, const fig::uuid& parent, bool bChecksum) noexcept
	{
		fig::uuid id = GenerateUUID();
		auto& asset = _assets[id] = Asset {};
		asset.id = id;
		asset.parent_id = parent;
		asset.type = type;
		asset.sync_state.file_sync = AssetSyncState::Status::Created;
		asset.sync_state.db_sync = AssetSyncState::Status::Created;
		asset.sync_state.has_meta = true;
		asset.sync_state.has_data = not data.empty();

		auto now = fig::now();
		asset.SetMeta(MetaTag::CreatedAt, now);
		asset.SetMeta(MetaTag::UpdatedAt, now);

		// Copy data
		asset.data.resize(data.size());
		std::memcpy(asset.data.data(), data.data(), data.size());

		if (bChecksum)
			asset.CalculateChecksum();
		return asset;
	}

	Asset& AssetManager::CreateImageAsset_NoLock(ImageAssetType subtype, const fig::sdl::Surface& surface, const fig::uuid& parent) noexcept
	{
		auto& asset = CreateAsset_NoLock(make_asset_type(AssetType::Image, subtype, DataFormat::ImageUncompressed), parent);
		if (!surface.get())
			return asset; // Error

		asset.sync_state.has_meta = true;
		asset.sync_state.has_data = true;

		auto pSurface = surface.get();
		int32_t stride = pSurface->pitch / pSurface->w;
		assert(stride == 3 or stride == 4);

		ImageFormat format;
		switch (stride)
		{
		case 3:
			format = ImageFormat::RGB24;
			break;
		case 4:
			format = ImageFormat::RGBA32;
			break;
		default:
			format = ImageFormat::Undefined;
		}

		asset.SetMeta(MetaTag::ImageWidth, static_cast<uint16_t>(pSurface->w));
		asset.SetMeta(MetaTag::ImageHeight, static_cast<uint16_t>(pSurface->h));
		asset.SetMeta(MetaTag::ImageFormat, static_cast<uint8_t>(format));

		if (SDL_LockSurface(pSurface))
		{
			size_t data_length = toUZ(pSurface->h * pSurface->pitch);

			// Copy pixel data
			asset.data.resize(data_length);
			std::memcpy(asset.data.data(), (fig::byte*)pSurface->pixels, data_length);
			SDL_UnlockSurface(pSurface);
		}
		asset.CalculateChecksum();
		return asset;
	}

	fig::optional_cref<Asset> AssetManager::FindAsset(const fig::uuid& id) noexcept
	{
		std::scoped_lock lock { _assetsMutex };
		return FindAsset_NoLock(id);
	}

	fig::optional_cref<Asset> AssetManager::FindAsset(const fig::uuid& id, AssetType assetType) noexcept
	{
		std::scoped_lock lock { _assetsMutex };
		return FindAsset_NoLock(id, assetType);
	}

	fig::optional_cref<Asset> AssetManager::FindAssetOfType(AssetTypeDefinition type, const fig::uuid& parentId) noexcept
	{
		std::scoped_lock lock { _assetsMutex };

		bool with_parent = !parentId.empty();

		auto itFind = std::find_if(_assets.begin(), _assets.end(),
			[&parentId, type, with_parent](auto& kvp) {
				return (not with_parent or kvp.second.parent_id == parentId)
					and kvp.second.type.IsOfType(type.GetType(), type.subtype);
			});

		if (itFind != _assets.cend())
			return make_optional_cref(itFind->second);
		return fig::nullref;
	}

	fig::optional_cref<Asset> AssetManager::FindAsset_NoLock(const fig::uuid& id) noexcept
	{
		auto itFind = _assets.find(id);
		if (itFind != _assets.cend())
			return make_optional_cref(itFind->second);
		return fig::nullref;
	}

	fig::optional_cref<Asset> AssetManager::FindAsset_NoLock(const fig::uuid& id, AssetType assetType) noexcept
	{
		auto itFind = _assets.find(id);
		if (itFind != _assets.cend() and itFind->second.type.IsOfType(assetType))
			return make_optional_cref(itFind->second);
		return fig::nullref;
	}

	fig::optional_cref<Asset> AssetManager::FindAsset_NoLock(const fig::uuid& id, AssetTypeDefinition assetType) noexcept
	{
		auto itFind = _assets.find(id);
		if (itFind != _assets.cend() and (itFind->second).type.IsOfType(assetType, false))
			return make_optional_cref(itFind->second);
		return fig::nullref;
	}

	fig::cref_vector<Asset> AssetManager::FindChildrenOf(const fig::uuid& parentId) noexcept
	{
		std::scoped_lock lock { _assetsMutex };

		fig::cref_vector<Asset> children;
		for (auto& kvp : _assets)
		{
			if (kvp.second.parent_id == parentId)
				children.push_back(std::cref(kvp.second));
		}
		return children;
	}

	bool AssetManager::HasChildren(const fig::uuid& assetId) noexcept
	{
		std::scoped_lock lock { _assetsMutex };

		for (auto& kvp : _assets)
		{
			if (kvp.second.parent_id == assetId)
				return true;
		}
		return false;
	}

	constexpr int MaxAssetDepth = 8;

	int32_t AssetManager::GetAssetDepth_NoLock(const fig::uuid& id, std::unordered_map<fig::uuid, int32_t>& depthCache, int32_t depth) const noexcept
	{
		if (depth >= MaxAssetDepth)
		{
			Log("Asset parent chain exceeded max depth, possible cycle");
			return 0;
		}
		if (auto it = depthCache.find(id); it != depthCache.end())
			return it->second;
		auto it = _assets.find(id);
		if (it == _assets.end() or it->second.parent_id.empty())
			return depthCache[id] = 0;
		return depthCache[id] = 1 + GetAssetDepth_NoLock(it->second.parent_id, depthCache, depth + 1);
	}

	bool AssetManager::SaveModifiedAssets()
	{
		std::scoped_lock lock { _assetsMutex };
		
		bool bSaved = false;

		// Write files
		for (auto& kvp : _assets)
		{
			auto& asset = kvp.second;
			if (asset.sync_state.ShouldWriteToDisk())
			{
				assert(asset.sync_state.has_data and not asset.data.empty());
				WriteAssetToDisk(asset);
				bSaved = true;
			}
		}

		// Write to index database
		auto changedAssets = _assets
			| std::views::filter([](auto& kvp) { return kvp.second.sync_state.ShouldWriteToDatabase(); })
			| std::views::transform([](auto& kvp) { return std::ref(kvp.second); })
			| std::ranges::to<std::vector>();

		if (not changedAssets.empty())
		{
			// Sort by ancestry
			std::unordered_map<fig::uuid, int> depthCache;
			std::ranges::stable_sort(changedAssets, [&](auto& a, auto& b) {
				return GetAssetDepth_NoLock(a.get().id, depthCache) < GetAssetDepth_NoLock(b.get().id, depthCache);
			});

			auto& db = GetDatabase();
			if (auto result = db.UpsertAssets(changedAssets))
			{
				for (auto& assetRef : changedAssets)
					assetRef.get().sync_state.db_sync = AssetSyncState::Status::Synchronized;

				LogLn(std::format("Wrote {} asset(s) to index database", result.value()));
			}
			else
			{
				LogLn("Error occurred when updating index database");
			}
		}

		return bSaved;
	}

	bool AssetManager::WriteAssetToDisk(Asset& asset)
	{
		auto file = asset.ToFile();
		AssetFileWriter writer(_profilePath, _profileAuthKey);
		if (auto error = writer.WriteFile(file); error == FileError::NoError)
		{
			asset.sync_state.file_sync = AssetSyncState::Status::Synchronized;
			return true;
		}
		return false;
	}

	size_t AssetManager::LoadIndexDatabase() noexcept
	{
		DEBUG_MEASURE_BEGIN("Load index database");
		auto& db = GetDatabase();

		// Fetch folders
		if (auto folders = db.FetchFolders(); folders.has_value())
		{
			std::scoped_lock lock { _assetsMutex };
			_folders = std::move(folders.value());
		}

		// Fetch assets
		if (auto assets = db.FetchAssets(); assets.has_value())
		{
			std::scoped_lock lock { _assetsMutex };
			_assets = std::move(assets.value());
		}
		DEBUG_MEASURE_END();
		return _assets.size();
	}

	bool AssetManager::LoadMetaData(AssetType assetType) noexcept
	{
		std::scoped_lock lock { _assetsMutex };

		// Read meta data of all asset files (in parallel)
		DEBUG_MEASURE_BEGIN(std::format("Load meta data (0x{:02X})", (int32_t)assetType));
		std::vector<Asset*> meta_assets = _assets
			| std::views::filter([assetType](auto& kvp) { return kvp.second.IsOfType(assetType); })
			| std::views::values
			| std::views::transform([](auto&& a) { return &a; })
			| std::ranges::to<std::vector>();

		std::for_each(std::execution::par_unseq,
			meta_assets.begin(), meta_assets.end(),
			[&](Asset* pAsset) {
				auto discard = LoadAssetMeta_NoLock(*pAsset);
			});
		DEBUG_MEASURE_END();

		// Purge missing assets from index
		auto missingAssets = _assets
			| std::views::filter([](auto& kvp) { return kvp.second.sync_state.error == AssetSyncState::Error::Missing; })
			| std::views::keys
			| std::ranges::to<std::vector>();

		if (not missingAssets.empty())
		{
			DEBUG_MEASURE_BEGIN("Remove missing assets");
			for (auto& id : missingAssets)
			{
				_assets.erase(id);
				_pAssetDB->DeleteAsset(id);
				LogLn(std::format("Removed missing asset {}", (fig::string)id));
			}
			DEBUG_MEASURE_END();
		}
		return true;
	}

	bool AssetManager::LoadAssetData() noexcept
	{
		std::scoped_lock lock { _assetsMutex };

		DEBUG_MEASURE_BEGIN("LoadAssets (Full)");
		std::vector<Asset*> assets = _assets
			| std::views::filter([](auto& kvp) { 
				return kvp.second.type.IsOfType(AssetType::Character) 
					|| kvp.second.type.IsOfType(AssetType::Scenario) 
					|| kvp.second.type.IsOfType(AssetType::Chat, ChatAssetType::Instance); })
			| std::views::values
			| std::views::transform([](auto&& a) { return &a; })
			| std::ranges::to<std::vector>();

		std::for_each(std::execution::par_unseq,
			assets.begin(), assets.end(),
			[&](Asset* pAsset) {
			auto discard = LoadAsset_NoLock(*pAsset);
		});
		DEBUG_MEASURE_END();
		return true;
	}

	void AssetManager::LoadAssetData(const std::vector<fig::uuid>& assetIds) noexcept
	{
		std::scoped_lock lock { _assetsMutex };

		std::vector<Asset*> assets = assetIds
			| std::views::transform([&](auto&& id) { return &_assets.at(id); })
			| std::ranges::to<std::vector>();

		std::for_each(std::execution::par_unseq,
			assets.begin(), assets.end(),
			[&](Asset* pAsset) {
			auto discard = LoadAsset_NoLock(*pAsset);
		});
	}

	void AssetManager::LoadAssetData(const fig::ref_vector<Asset>& assets) noexcept
	{
		std::scoped_lock lock { _assetsMutex };

		std::for_each(std::execution::par_unseq,
			assets.cbegin(), assets.cend(),
			[&](auto&& asset) {
			auto discard = LoadAsset_NoLock(asset.get());
		});
	}

	FileError AssetManager::LoadAsset(const Asset& asset) noexcept
	{
		if (const auto result = LoadAsset(asset.id); result.has_value())
			return FileError::NoError;
		else
			return result.error();
	}

	fig::expected_cref<Asset, FileError> AssetManager::LoadAsset(const fig::uuid& id) noexcept
	{
		std::scoped_lock lock { _assetsMutex };
		auto itFind = _assets.find(id);
		if (itFind == _assets.cend())
			return std::unexpected(FileError::NotFound);

		Asset& asset = itFind->second;
		if (asset.sync_state.has_data)
			return asset;

		return LoadAsset_NoLock(asset);
	}

	fig::expected_ref<Asset, FileError> AssetManager::LoadAsset_NoLock(Asset& asset) noexcept
	{
		if (asset.sync_state.has_data)
			return asset;

		if (asset.sync_state.error != AssetSyncState::Error::NoError)
			return std::unexpected(FileError::ReadError);

		AssetFileReader reader(_profilePath, _profileAuthKey);
		if (auto file = reader.ReadFile(asset.GetFileName()))
		{
			if (file.value().data.empty())
			{
//				auto k = asset.GetFileName();
				asset.sync_state.error = AssetSyncState::Error::Invalid;
				return std::unexpected(FileError::ReadError);
			}

			asset.FromFile(std::move(file.value()));
			asset.sync_state.file_sync = AssetSyncState::Status::Synchronized;
			asset.sync_state.has_meta = true;
			asset.sync_state.has_data = not asset.data.empty();
			return asset;
		}
		else if (file.error() == FileError::NotFound)
		{
			asset.sync_state.error = AssetSyncState::Error::Missing;
			return std::unexpected(FileError::ReadError);
		}
		else
		{
			asset.sync_state.error = AssetSyncState::Error::Invalid;
			return std::unexpected(FileError::ReadError);
		}
	}

	fig::expected_ref<Asset, FileError> AssetManager::LoadAssetMeta_NoLock(Asset& asset) noexcept
	{
		if (asset.sync_state.has_meta)
			return asset;

		if (asset.sync_state.error != AssetSyncState::Error::NoError)
			return std::unexpected(FileError::ReadError);

		AssetFileReader reader(_profilePath, _profileAuthKey);
		if (auto file = reader.ReadFile(asset.GetFileName(), false))
		{
			asset.FromFile(std::move(file.value()));
			asset.sync_state.file_sync = AssetSyncState::Status::Synchronized;
			asset.sync_state.has_meta = true;
			asset.sync_state.has_data = false;
			return asset;
		}
		else if (file.error() == FileError::NotFound)
		{
			asset.sync_state.error = AssetSyncState::Error::Missing;
			return std::unexpected(FileError::ReadError);
		}
		else
		{
			asset.sync_state.error = AssetSyncState::Error::Invalid;
			return std::unexpected(FileError::ReadError);
		}
	}

	std::expected<Character, FileError> AssetManager::LoadCharacterData(fig::path filename, CharacterDataFormat format)
	{
		switch (format)
		{
			case CharacterDataFormat::Default:
			{
				Character character;
				if (auto error = character.LoadFromXml(filename); error == FileError::NoError)
					return character;
				else
					return std::unexpected(error);
			}
			case CharacterDataFormat::TavernV2:
				if (auto import = CardImporter::Import(filename))
					return import.value();
				else
					return std::unexpected(import.error());
			default:
				return std::unexpected(FileError::UnrecognizedFormat);
		}
		return std::unexpected(FileError::UnrecognizedFormat);
	}

	bool AssetManager::DeleteAsset(fig::uuid assetId) noexcept
	{
		return DeleteAssets(make_span(assetId)) != 0uz;
	}

	size_t AssetManager::DeleteAssets(std::span<fig::uuid> assetIds) noexcept
	{
		if (assetIds.empty())
			return 0uz;
		else
		{
			std::scoped_lock lock { _assetsMutex };
			return DeleteAssets_NoLock(assetIds);
		}
	}

	size_t AssetManager::DeleteAssets_NoLock(std::span<fig::uuid> assetIds) noexcept
	{
		auto& db = GetDatabase();
		size_t db_deletions = 0;
		size_t file_deletions = 0;
		if (auto result = db.DeleteAssets(assetIds))
		{
			db_deletions = result.value();

			std::unordered_set<fig::uuid> assetFileIds;
			for (auto& assetID : assetIds)
				assetFileIds.insert_range(FindRelatedAssets_NoLock(assetID));
			file_deletions = DeleteAssetFiles_NoLock(make_span(assetFileIds));
		}
		
		LogLn(std::format("Deleted {} assets (and {} files)", db_deletions, file_deletions));
		return db_deletions;
	}

	size_t AssetManager::DeleteAssetFiles_NoLock(std::span<fig::uuid> assetIds) noexcept
	{
		size_t count = 0uz;
		for (auto& assetId : assetIds)
		{
			fig::path path = _profilePath / filename_from_uuid(assetId);
			if (std::filesystem::exists(path) and std::filesystem::is_regular_file(path))
			{
				std::error_code err;
				if (std::filesystem::remove(path, err))
					count++;
			}
		}
		return count;
	}

	bool AssetManager::ReleaseAssetData(const fig::uuid& assetId) noexcept
	{
		std::scoped_lock lock { _assetsMutex };
		if (auto it = _assets.find(assetId); it != _assets.end())
		{
			auto& asset = it->second;
			asset.data.clear();
			asset.sync_state.has_data = false;
			asset.sync_state.file_sync = AssetSyncState::Status::Indeterminate;
			return true;
		}

		return false;
	}

	std::unordered_set<fig::uuid> AssetManager::FindRelatedAssets_NoLock(const fig::uuid& assetID) noexcept
	{
		std::unordered_set<fig::uuid> assetIDs;
		std::unordered_set<fig::uuid> openList;

		// Find all related assets
		openList.insert(assetID);

		while (not openList.empty())
		{
			fig::uuid id = *openList.begin();
			openList.erase(id);
			if (auto it = _assets.find(id); it == _assets.end())
				continue;

			assetIDs.insert(id);

			// Scan children
			openList.insert_range(_assets
				| std::views::filter([&id](auto const& kvp) { return kvp.second.parent_id == id; })
				| std::views::transform([](auto const& kvp) -> fig::uuid { return kvp.second.id; }));
		}

		return assetIDs;
	}

	fig::ref_vector<Asset> AssetManager::ImportCharactersInDirectory(const fig::path& directory, CharacterDataFormat format, size_t max_count)
	{
		std::vector<fig::path> files;
		for (const auto& entry : std::filesystem::directory_iterator(directory))
			files.push_back(entry.path());

		if (max_count > 0)
			files.resize(std::min(files.size(), max_count));

		fig::ref_vector<Asset> imported;
		imported.reserve(files.size());

		{	// Mutex scope
			std::scoped_lock lock { _assetsMutex };
			for (auto& filename : files)
			{
				if (auto import = ImportCharacter_NoLock(filename, format))
					imported.push_back(std::ref(import.value()));
			}
		}

		return imported;
	}

	fig::expected_ref<Asset, FileError> AssetManager::ImportCharacter(const fig::path& filename, CharacterDataFormat format)
	{
		std::scoped_lock lock { _assetsMutex };
		return ImportCharacter_NoLock(filename, format);
	}

	fig::expected_ref<Asset, FileError> AssetManager::ImportCharacter_NoLock(const fig::path& filename, CharacterDataFormat format)
	{
		if (auto try_character = LoadCharacterData(filename, format))
		{
			auto& character = try_character.value();

			// Create asset
			fig::bytes characterData;
			character.SaveToXml(characterData);
			auto& characterAsset = CreateAsset_NoLock(make_asset_type(AssetType::Character, DataFormat::TextXml), characterData, {}, false);

			// Load portrait image(s)
			if (auto file = fig::io::ReadFile(filename))
			{
				// Create portrait asset
				auto& portraitAsset = CreateAsset_NoLock(make_asset_type(AssetType::Image, ImageAssetType::LargePortrait, DataFormat::ImagePng), std::move(file.value()), characterAsset.id, true);

				// Create cover card
				if (auto coverImage = LoadImage(filename) //! @todo: load only once
					.transform([](auto img) {
					return CreateCoverImage(img, false);
				}))
				{
					// Save cover asset (bitmap)
					auto& coverAsset = CreateImageAsset_NoLock(ImageAssetType::CoverImage, coverImage.value(), characterAsset.id);
					coverAsset.SetMeta(MetaTag::ReferenceToOriginal, portraitAsset.id);
				}

				// Create square portrait
				if (auto squarePortraitImage = LoadImage(filename) //! @todo: load only once
					.transform([](auto img) {
					return CreateSquarePortrait(img);
				}))
				{
					// Save square portrait asset (bitmap)
					auto& squarePortraitAsset = CreateImageAsset_NoLock(ImageAssetType::SmallPortrait, squarePortraitImage.value(), characterAsset.id);
					squarePortraitAsset.SetMeta(MetaTag::ReferenceToOriginal, portraitAsset.id);
				}
			}

			LogLn(std::format("Imported {}", filename.filename().u8string().c_str()));
			return characterAsset;
		}
		else
			return std::unexpected(try_character.error());
	}

	fig::expected_ref<Asset, FileError> AssetManager::ImportScenario(const fig::path& filename)
	{
		std::scoped_lock lock { _assetsMutex };
		return ImportScenario_NoLock(filename);
	}

	fig::expected_ref<Asset, FileError> AssetManager::ImportScenario_NoLock(const fig::path& filename)
	{
		Scenario scenario;
		if (auto error = scenario.LoadFromXml(filename); error != FileError::NoError)
			return std::unexpected(error);

		// Create asset
		fig::bytes scenarioData;
		scenario.SaveToXml(scenarioData);
		auto& scenarioAsset = CreateAsset_NoLock(make_asset_type(AssetType::Scenario, DataFormat::TextXml), scenarioData, {}, false);

		// Load scenario image
/*		if (not empty_or_whitespace(scenario.imageFilename))
		{
			if (auto file = fig::io::ReadFile(filename.parent_path() / scenario.imageFilename))
			{
				// Create portrait asset
				auto& scenarioImageAsset = CreateImageAsset_NoLock(ImageAssetType::Undefined, DataFormatFromExt(GetFileExt(scenario.imageFilename)), std::move(file.value()), scenarioAsset.id);

				// Create cover card
				if (auto coverImage = LoadImage(filename.parent_path() / scenario.imageFilename)
					.transform([](auto img) {
					return CreateCoverImage(img, false);
				}))
				{
					// Save cover asset (bitmap)
					auto& coverAsset = CreateImageAsset_NoLock(ImageAssetType::CoverImage, coverImage.value(), scenarioAsset.id);
					coverAsset.SetMeta(MetaTag::ReferenceToOriginal, scenarioImageAsset.id);
				}
			}
		} */

		LogLn(std::format("Imported {}", filename.filename().u8string().c_str()));
		return scenarioAsset;
	}

	FileError AssetManager::CreateProfilePicture(const fig::user::UserProfile& profile, fig::path filename)
	{
		// Create profile image
		if (auto profileImage = LoadImage(filename)
			.transform([](auto img) { return CreateProfileImage(img); });
			profileImage.has_value() && profileImage.value()->w > 0)
		{
			auto pSurface = (*profileImage).get();
			fig::bytes data;
			if (SDL_LockSurface(pSurface))
			{
				size_t data_length = toUZ(pSurface->h * pSurface->pitch);

				// Copy pixel data
				data.resize(data_length);
				std::memcpy(data.data(), (fig::byte*)pSurface->pixels, data_length);
				SDL_UnlockSurface(pSurface);
			}
			else
			{
				return FileError::WriteError;
			}

			AssetFile image_file {
				.parent_id {profile.id},
				.type = make_asset_type(AssetType::Image, ImageAssetType::ProfileImage, DataFormat::ImageUncompressed),
				.data_length { data.size() },
				.data_encrypted { false },
				.data { std::move(data) },
			};
			auto now = fig::now();
			image_file.meta[MetaTag::CreatedAt] = now;
			image_file.meta[MetaTag::UpdatedAt] = now;
			image_file.meta[MetaTag::ImageWidth] = static_cast<uint16_t>(pSurface->w);
			image_file.meta[MetaTag::ImageHeight] = static_cast<uint16_t>(pSurface->h);
			image_file.meta[MetaTag::ImageFormat] = static_cast<uint8_t>(0x04);

			auto filename = fig::path(std::format("{}.{}", Constants::Paths::ProfileImageFileName, Constants::Paths::ProfileImageFileExt));
			return AssetFileWriter::WriteProfileFile(profile, filename, image_file);
		}

		return FileError::UnknownError;
	}

	fig::uuid AssetManager::GenerateUUID() const noexcept
	{
		auto uuid = _CreateUUID();
		while (_assets.contains(uuid))
			uuid = _CreateUUID();
		return uuid;
	}

	IndexDatabase& AssetManager::GetDatabase() noexcept
	{
		if (!_pAssetDB)
			_pAssetDB = std::make_unique<IndexDatabase>(_profilePath / fig::path(std::format("{}.{}", Constants::Paths::AssetIndexFileName, Constants::Paths::AssetIndexFileExt)));
		return *_pAssetDB.get();
	}

	static bool CreateSurface(const Asset& cover, fig::sdl::Surface& out_surface)
	{
		// Create SDL surface
		int32_t width = cover.GetMeta<uint16_t>(MetaTag::ImageWidth).value_or(Constants::GUI::CardWidth);
		int32_t height = cover.GetMeta<uint16_t>(MetaTag::ImageHeight).value_or(Constants::GUI::CardHeight);
		ImageFormat format = static_cast<ImageFormat>(cover.GetMeta<uint8_t>(MetaTag::ImageFormat).value_or(0));

		try
		{
			fig::surface_ptr pSurface = SDL_CreateSurface(width, height, to_sdl_format(format));
			if (!pSurface)
				return false;

			if (pSurface->pitch * pSurface->h != cover.data.size())
				return {}; // Invalid data length

			if (SDL_LockSurface(pSurface))
			{
				std::memcpy(pSurface->pixels, cover.data.data(), cover.data.size());
				SDL_UnlockSurface(pSurface);

				out_surface.reset(pSurface);
				return true;
			}
		}
		catch (...)
		{
		}
		return false;
	}

	AsyncLoadError AssetManager::__LoadImageTask(const fig::uuid& characterAssetID, ImageAssetType imageType, AsyncResultVariant& outResult) noexcept
	{
		if (auto findImage = FindAssetOfType(make_asset_type(AssetType::Image, imageType), characterAssetID))
		{
			auto& imageAsset = findImage.value();
			if (auto result = LoadAsset(imageAsset); result == FileError::NoError)
			{
				if (auto image = LoadImageFromMemory(imageAsset.data))
				{
					outResult.emplace<AsyncResult_Image>(std::move(image.value()));
					return AsyncLoadError::NoError;
				}
			}
			else if (result == FileError::NotFound)
				return AsyncLoadError::FileNotFound;
			else
				return AsyncLoadError::LoadError;
		}

		return AsyncLoadError::FileNotFound;
	}

	AsyncLoadError AssetManager::__LoadCoverImageTask(const fig::uuid& characterAssetID, AsyncResultVariant& outResult) noexcept
	{
		fig::sdl::Surface fullSurface {};
		fig::sdl::Surface halfSurface {};

		if (auto findCover = FindAssetOfType(make_asset_type(AssetType::Image, ImageAssetType::CoverImage), characterAssetID))
		{
			auto& cover = findCover.value();
			auto result = LoadAsset(cover);
			if (result == FileError::NotFound)
				return AsyncLoadError::FileNotFound;
			else if (result != FileError::NoError)
				return AsyncLoadError::LoadError;

			CreateSurface(cover, fullSurface);
		}

		if constexpr (Disabled)
		{
			if (fullSurface.empty())
			{
				// Create cover from portrait
				if (auto findPortrait = FindAssetOfType(make_asset_type(AssetType::Image, ImageAssetType::LargePortrait), characterAssetID))
				{
					auto& portraitAsset = findPortrait.value();
					if (LoadAsset(portraitAsset) == FileError::NoError)
					{
						if (auto portraitImage = LoadImageFromMemory(portraitAsset.data))
						{
							auto coverImage = ScaleSurface(portraitImage.value(), Constants::GUI::CardWidth, Constants::GUI::CardHeight, ImageFit::Portrait);

							// Save cover asset (bitmap)
							{
								std::scoped_lock lock { _assetsMutex };
								auto& coverAsset = CreateImageAsset_NoLock(ImageAssetType::CoverImage, coverImage, characterAssetID);
								coverAsset.SetMeta(MetaTag::Version, uint8_t { 1 });
								coverAsset.SetMeta(MetaTag::ReferenceToOriginal, portraitAsset.id);
							}

							fullSurface = std::move(coverImage);
						}
					}
				}
			}
		}

		if (fullSurface.empty())
			return AsyncLoadError::FileNotFound;

		// Half-version
		int32_t expandX = Constants::GUI::Cards::Half::ZoomPixels * 2;
		int32_t expandY = toI(std::ceilf(toF(Constants::GUI::Cards::Half::ZoomPixels * 2) * toF(Constants::GUI::HalfCardHeight) / toF(Constants::GUI::HalfCardWidth)));
		halfSurface = ScaleSurface(fullSurface, Constants::GUI::HalfCardWidth + expandX, Constants::GUI::HalfCardHeight + expandY, ImageFit::Stretch, true);

		// Round corners
		if constexpr (Disabled)
		{
			MaskCorners(halfSurface, MaskType::CARD_CORNER_MASK);
			MaskCorners(fullSurface, MaskType::CARD_CORNER_MASK);
		}

		outResult.emplace<AsyncResult_CoverPair>(AsyncResult_CoverPair {
			std::move(fullSurface),
			std::move(halfSurface),
			});
		return AsyncLoadError::NoError;
	}

	void AssetManager::__Worker(std::stop_token stop)
	{
		PendingRequest request;
		while (not stop.stop_requested())
		{
			// Block until next task
			{
				std::unique_lock lock(_pending_mutex);
				_pending_cv.wait(lock, [&] {
					return !_pending.empty() || stop.stop_requested();
				});
				if (stop.stop_requested())
					break; // Stop

				request = std::move(const_cast<PendingRequest&>(_pending.top()));
				_pending.pop();
			}

			if (not IsAsyncRequestAlive(request))
			{
				request.promise->set_value(std::unexpected(AsyncLoadError::Canceled));
				continue;
			}

			if constexpr (Disabled)
			{
				// Simulated slowness
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}

			// Do work
			AsyncLoadError error;
			AsyncResultVariant result;
			switch (request.task)
			{
				case AsyncTask::LoadCoverImage:
					error = __LoadCoverImageTask(request.assetId, result);
					break;
				case AsyncTask::LoadPortrait:
					error = __LoadImageTask(request.assetId, ImageAssetType::LargePortrait, result);
					break;
				default:
					error = AsyncLoadError::LoadError;
					break;
			}

			if (IsAsyncRequestAlive(request))
			{
				std::scoped_lock<std::mutex> lock(_active_mutex);
				_active_promises.erase(request.assetId);
			}
			else
			{
				// Canceled
				request.promise->set_value(std::unexpected(AsyncLoadError::Canceled));
				continue;
			}

			if (error == AsyncLoadError::NoError)
				request.promise->set_value(std::move(result));
			else
				request.promise->set_value(std::unexpected(error));
		}
	}

	[[nodiscard]] AsyncLoad AssetManager::LoadAssetAsync(const fig::uuid& assetId, AsyncTask task, int32_t priority)
	{
		if (task == AsyncTask::None)
		{
			return AsyncLoad {
				.assetId = assetId,
				.task = task,
				.future = {},
			};
		}

		const uint64_t id = _next_id.fetch_add(1, std::memory_order_relaxed);

		// Cancel the previous request for this card, if any
		{
			std::scoped_lock lock(_active_mutex);
			if (auto it = _active_promises.find(assetId); it != _active_promises.end())
			{
				it->second->set_value(std::unexpected(AsyncLoadError::Canceled));
				_active_promises.erase(it);
			}
		}

		// Create the promise
		auto promise = std::make_unique<AsyncPromise>();
		auto future = promise->get_future();
		auto promise_ptr = promise.get();

		{	// Store promise
			std::scoped_lock lock(_active_mutex);
			_active_promises[assetId] = promise_ptr;
		}

		{	// Enqueue request
			std::scoped_lock lock(_pending_mutex);
			_pending.push(PendingRequest {
				.id = id,
				.assetId = assetId,
				.priority = priority,
				.task = task,
				.promise = std::move(promise),
			});
		}
		_pending_cv.notify_one();

		return AsyncLoad {
			.id = id,
			.assetId = assetId,
			.task = task,
			.future = std::move(future),
		};
	}

	void AssetManager::Cancel(const fig::uuid& assetId)
	{
		std::scoped_lock lock(_active_mutex);
		if (auto it = _active_promises.find(assetId); it != _active_promises.end())
		{
			it->second->set_value(std::unexpected(AsyncLoadError::Canceled));
			_active_promises.erase(it);
		}
	}

	void AssetManager::CancelAll()
	{
		std::scoped_lock lock(_pending_mutex);
		while (!_pending.empty())
		{
			PendingRequest request = std::move(const_cast<PendingRequest&>(_pending.top()));
			_pending.pop();
			request.promise->set_value(std::unexpected(AsyncLoadError::Canceled));
		}
		_pending_cv.notify_all();
	}

	bool AssetManager::IsAsyncRequestAlive(const PendingRequest& request) const
	{
		std::scoped_lock lock(_active_mutex);
		auto it = _active_promises.find(request.assetId);
		if (it == _active_promises.cend())
			return false;
		return it != _active_promises.end();
	}

	void AssetManager::ModifyAsset_Void(const fig::uuid& assetId, std::function<void(Asset&)> fn)
	{
		if (auto itFind = _assets.find(assetId); itFind != _assets.cend())
		{
			if (fn)
				fn(itFind->second);
		}
	}

	void AssetManager::ModifyAsset_Void(const Asset& asset, std::function<void(Asset&)> fn)
	{
		if (fn)
			fn(const_cast<Asset&>(asset));
	}

	bool AssetManager::ModifyAsset_Bool(const fig::uuid& assetId, std::function<bool(Asset&)> fn)
	{
		if (auto itFind = _assets.find(assetId); itFind != _assets.cend())
		{
			if (fn)
				return fn(itFind->second);
		}
		return false;
	}

	bool AssetManager::ModifyAsset_Bool(const Asset& asset, std::function<bool(Asset&)> fn)
	{
		if (fn)
			return fn(const_cast<Asset&>(asset));
		return false;
	}

	void AssetManager::__Autosave(std::stop_token stopToken, std::chrono::seconds interval)
	{
		std::mutex wait_mutex;
		std::unique_lock wait_lock(wait_mutex);
		while (not stopToken.stop_requested())
		{
			_autosave_cv.wait_for(wait_lock, stopToken, interval, [] { return false; });
			if (not stopToken.stop_requested())
			{
				if (SaveModifiedAssets())
					LogLn("Auto-saved");
			}
		}
	}

	std::set<fig::uuid> AssetManager::GetAssociatedAssets(const fig::uuid& assetId) noexcept
	{
		std::scoped_lock lock { _assetsMutex };

		std::set<fig::uuid> result;
		std::set<fig::uuid> openList;

		openList.insert(assetId);

		if (auto try_asset = FindAsset_NoLock(assetId))
		{
			auto& asset = *try_asset;
			// References
			for (uint8_t idx = static_cast<uint8_t>(MetaTag::ReferenceToCharacter); idx < static_cast<uint8_t>(MetaTag::ReferenceToUser); ++idx)
			{
				if (auto ref = asset.GetMeta<fig::uuid>(static_cast<MetaTag>(idx)))
					openList.insert(*ref);
			}
			if (auto ref = asset.GetMeta<fig::uuid>(MetaTag::ReferenceToUser))
				openList.insert(*ref);
			if (auto ref = asset.GetMeta<fig::uuid>(MetaTag::ReferenceToScenario))
				openList.insert(*ref);
			if (auto ref = asset.GetMeta<fig::uuid>(MetaTag::ReferenceToWorld))
				openList.insert(*ref);
		}

		while (not openList.empty())
		{
			fig::uuid id = *openList.begin();
			openList.erase(id);
			if (id.empty())
				continue;
			if (auto it = _assets.find(id); it == _assets.end())
				continue;

			result.insert(id);

			// Scan children
			openList.insert_range(_assets
				| std::views::filter([&id](auto const& kvp) { return kvp.second.parent_id == id; })
				| std::views::transform([](auto const& kvp) -> fig::uuid { return kvp.second.id; }));
		}

		return result;
	}
}