#include <pch.h>
#include <filesystem>
#include <format>
#include <ranges>
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
	AssetManager::AssetManager(const fig::user::UserProfile& profile, const fig::auth::AuthKey& authKey, int32_t worker_threads)
	{
		_profileAuthKey = authKey;
		_profileID = profile.id;
		_profilePath = profile.GetPath();

		if (LoadAssetIndex())
		{
			LoadMetaData(AssetType::Character);
			LoadMetaData(AssetType::Scenario);
			LoadMetaData(AssetType::ChatInstance);
		}
		else
		{
			Log(std::format("No asset index found for profile '{}'.", profile.name));
		}

		_workers.reserve(worker_threads);
		for (int i = 0; i < worker_threads; ++i)
			_workers.emplace_back([this](std::stop_token stop) { __Worker(stop); });
	}

	AssetManager::~AssetManager()
	{
		{
			std::scoped_lock lock(_pending_mutex);
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
	}

	const Asset& AssetManager::CreateEmptyAsset(AssetType type, const fig::uuid& parent) noexcept
	{
		std::scoped_lock lock { _assetsMutex };

		return CreateEmptyAsset_Internal(type, parent);
	}

	Asset& AssetManager::CreateEmptyAsset_Internal(AssetType type, const fig::uuid& parent) noexcept
	{
		fig::uuid id = GenerateUUID();
		auto& asset = _assets[id] = Asset {};
		asset.id = id;
		asset.parent_id = not parent.empty() ? parent : _profileID;
		asset.asset_type = type;
		asset.sync_state.file_sync = AssetSyncState::Status::Created;
		asset.sync_state.db_sync = AssetSyncState::Status::Created;
		asset.sync_state.has_meta = true;

		auto now = utc_now();
		asset.SetMeta(MetaTag::CreatedAt, now);
		asset.SetMeta(MetaTag::UpdatedAt, now);
		asset.SetMeta(MetaTag::LastUsedAt, now);
		return asset;
	}
	
	const Asset& AssetManager::CreateAsset(AssetType type, fig::bytes&& data, const fig::uuid& parent) noexcept
	{
		std::scoped_lock lock { _assetsMutex };

		return CreateAsset_Internal(type, DataFormat::Undefined, std::move(data), parent);
	}

	const Asset& AssetManager::CreateAsset(AssetType type, fig::byte_span data, const fig::uuid& parent) noexcept
	{
		std::scoped_lock lock { _assetsMutex };

		return CreateAsset_Internal(type, DataFormat::Undefined, data, parent);
	}
	
	const Asset& AssetManager::CreateAsset(AssetType type, DataFormat format, fig::bytes&& data, const fig::uuid& parent) noexcept
	{
		std::scoped_lock lock { _assetsMutex };

		return CreateAsset_Internal(type, format, std::move(data), parent);
	}

	const Asset& AssetManager::CreateAsset(AssetType type, DataFormat format, fig::byte_span data, const fig::uuid& parent) noexcept
	{
		std::scoped_lock lock { _assetsMutex };

		return CreateAsset_Internal(type, format, data, parent);
	}

	Asset& AssetManager::CreateAsset_Internal(AssetType type, DataFormat format, fig::bytes&& data, const fig::uuid& parent) noexcept
	{
		fig::uuid id = GenerateUUID();
		auto& asset = _assets[id] = Asset {};
		asset.id = id;
		asset.parent_id = not parent.empty() ? parent : _profileID;
		asset.asset_type = type;
		asset.data = std::move(data); // Move data
		asset.data_format = format;
		asset.sync_state.file_sync = AssetSyncState::Status::Created;
		asset.sync_state.db_sync = AssetSyncState::Status::Created;
		asset.sync_state.has_meta = true;
		asset.sync_state.has_data = not asset.data.empty();
		auto now = utc_now();
		asset.SetMeta(MetaTag::CreatedAt, now);
		asset.SetMeta(MetaTag::UpdatedAt, now);
		asset.SetMeta(MetaTag::LastUsedAt, now);
		return asset;
	}

	Asset& AssetManager::CreateAsset_Internal(AssetType type, DataFormat format, fig::byte_span data, const fig::uuid& parent) noexcept
	{
		fig::uuid id = GenerateUUID();
		auto& asset = _assets[id] = Asset {};
		asset.id = id;
		asset.parent_id = not parent.empty() ? parent : _profileID;
		asset.asset_type = type;
		asset.data_format = format;
		asset.sync_state.file_sync = AssetSyncState::Status::Created;
		asset.sync_state.db_sync = AssetSyncState::Status::Created;
		asset.sync_state.has_meta = true;
		asset.sync_state.has_data = not data.empty();

		auto now = utc_now();
		asset.SetMeta(MetaTag::CreatedAt, now);
		asset.SetMeta(MetaTag::UpdatedAt, now);
		asset.SetMeta(MetaTag::LastUsedAt, now);

		// Copy data
		asset.data.resize(data.size());
		std::memcpy(asset.data.data(), data.data(), data.size());
		return asset;
	}

	const Asset& AssetManager::CreateImageAsset(ImageType subtype, DataFormat format, fig::bytes&& data, const fig::uuid& parent) noexcept
	{
		std::scoped_lock lock { _assetsMutex };
		return CreateImageAsset_Internal(subtype, format, std::move(data), parent);
	}

	Asset& AssetManager::CreateImageAsset_Internal(ImageType subtype, DataFormat format, fig::bytes&& data, const fig::uuid& parent) noexcept
	{
		auto& asset = CreateAsset_Internal(AssetType::Image, format, std::move(data), parent);
		asset.asset_subtype = static_cast<uint8_t>(subtype);
		asset.CalculateChecksum();
		return asset;
	}

	const Asset& AssetManager::CreateImageAsset(ImageType subtype, DataFormat format, fig::byte_span data, const fig::uuid& parent) noexcept
	{
		std::scoped_lock lock { _assetsMutex };

		auto& asset = CreateAsset_Internal(AssetType::Image, format, data, parent);
		asset.asset_subtype = static_cast<uint8_t>(subtype);
		asset.CalculateChecksum();
		return asset;
	}

	const Asset& AssetManager::CreateImageAsset(ImageType subtype, const fig::sdl::Surface& surface, const fig::uuid& parent) noexcept
	{
		std::scoped_lock lock { _assetsMutex };
		return CreateImageAsset_Internal(subtype, surface, parent);
	}

	Asset& AssetManager::CreateImageAsset_Internal(ImageType subtype, const fig::sdl::Surface& surface, const fig::uuid& parent) noexcept
	{
		auto& asset = CreateEmptyAsset_Internal(AssetType::Image, parent);
		if (!surface.get())
			return asset; // Error

		asset.asset_subtype = static_cast<uint8_t>(subtype);
		asset.data_format = DataFormat::ImageUncompressed;
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
		return FindAsset_Internal(id);
	}

	fig::optional_cref<Asset> AssetManager::FindAsset(const fig::uuid& id, AssetType assetType) noexcept
	{
		std::scoped_lock lock { _assetsMutex };
		return FindAsset_Internal(id, assetType);
	}

	fig::optional_cref<Asset> AssetManager::FindImageAsset(const fig::uuid& parentId, ImageType imageType) noexcept
	{
		std::scoped_lock lock { _assetsMutex };

		auto itFind = std::find_if(_assets.begin(), _assets.end(),
			[&parentId, imageType](auto& kvp) {
				return kvp.second.parent_id == parentId
					and kvp.second.asset_type == AssetType::Image 
					and kvp.second.asset_subtype == static_cast<uint8_t>(imageType);
			});

		if (itFind != _assets.cend())
			return make_optional_cref(itFind->second);
		return fig::nullref;
	}

	fig::optional_cref<Asset> AssetManager::FindAsset_Internal(const fig::uuid& id) noexcept
	{
		auto itFind = _assets.find(id);
		if (itFind != _assets.cend())
			return make_optional_cref(itFind->second);
		return fig::nullref;
	}

	fig::optional_cref<Asset> AssetManager::FindAsset_Internal(const fig::uuid& id, AssetType assetType) noexcept
	{
		auto itFind = _assets.find(id);
		if (itFind != _assets.cend() and itFind->second.asset_type == assetType)
			return make_optional_cref(itFind->second);
		return fig::nullref;
	}

	void AssetManager::SaveModified()
	{
		std::scoped_lock lock { _assetsMutex };

		for (auto& kvp : _assets)
		{
			auto& asset = kvp.second;
			if (asset.sync_state.error == AssetSyncState::Error::NoError)
			{
				if (asset.sync_state.ShouldWriteToDisk())
				{
					assert(asset.sync_state.has_data and not asset.data.empty());
					UpdateAssetOnDisk(asset);
				}
				if (asset.sync_state.ShouldWriteToDatabase())
					UpdateAssetInDatabase(asset);
			}
		}
	}

	bool AssetManager::UpdateAssetOnDisk(Asset& asset)
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

	bool AssetManager::UpdateAssetInDatabase(Asset& asset)
	{
		auto& db = GetDatabase();
		if (asset.sync_state.db_sync == AssetSyncState::Status::Created)
		{
			if (db.CreateAsset(asset) == DatabaseError::NoError)
			{
				asset.sync_state.db_sync = AssetSyncState::Status::Synchronized;
				return true;
			}
		}
		else if (asset.sync_state.db_sync == AssetSyncState::Status::Modified)
		{
			if (db.UpdateAsset(asset) == DatabaseError::NoError)
			{
				asset.sync_state.db_sync = AssetSyncState::Status::Synchronized;
				return true;
			}
		}
		else
		{
			assert(false && "Invalid asset syncronization state");
		}
		return false;
	}

	size_t AssetManager::LoadAssetIndex() noexcept
	{
		DEBUG_MEASURE_BEGIN("LoadAssetIndex");
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
		DEBUG_MEASURE_BEGIN("Load meta data (all)");
		std::vector<Asset*> meta_assets = _assets
			| std::views::filter([assetType](auto& kvp) { return kvp.second.asset_type == assetType; })
			| std::views::values
			| std::views::transform([](auto&& a) { return &a; })
			| std::ranges::to<std::vector>();

		std::for_each(std::execution::par_unseq,
			meta_assets.begin(), meta_assets.end(),
			[&](Asset* pAsset) {
				auto discard = LoadAssetMeta_Internal(*pAsset);
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
			| std::views::filter([](auto& kvp) { return kvp.second.asset_type == AssetType::Character || kvp.second.asset_type == AssetType::Scenario || kvp.second.asset_type == AssetType::ChatInstance; })
			| std::views::values
			| std::views::transform([](auto&& a) { return &a; })
			| std::ranges::to<std::vector>();

		std::for_each(std::execution::par_unseq,
			assets.begin(), assets.end(),
			[&](Asset* pAsset) {
			auto discard = LoadAsset_Internal(*pAsset);
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
			auto discard = LoadAsset_Internal(*pAsset);
		});
	}

	void AssetManager::LoadAssetData(const fig::ref_vector<Asset>& assets) noexcept
	{
		std::scoped_lock lock { _assetsMutex };

		std::for_each(std::execution::par_unseq,
			assets.cbegin(), assets.cend(),
			[&](auto&& asset) {
			auto discard = LoadAsset_Internal(asset.get());
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

		return LoadAsset_Internal(asset);
	}

	fig::expected_ref<Asset, FileError> AssetManager::LoadAsset_Internal(Asset& asset) noexcept
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
				auto k = asset.GetFileName();
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

	fig::expected_ref<Asset, FileError> AssetManager::LoadAssetMeta_Internal(Asset& asset) noexcept
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

	bool AssetManager::DeleteAsset(const fig::uuid& assetID) noexcept
	{
		std::scoped_lock lock { _assetsMutex };
		return DeleteAsset_Internal(assetID);
	}

	uint32_t AssetManager::DeleteAssets(std::span<fig::uuid> assetIDs) noexcept
	{
		std::scoped_lock lock { _assetsMutex };
		std::set<fig::uuid> allAssetIDs;
		for (auto& assetID : assetIDs)
			allAssetIDs.insert_range(FindRelatedAssets(assetID));

		uint32_t count = 0;
		auto& db = GetDatabase();
		for (auto& assetID : allAssetIDs)
		{
			DeleteAssetFile(assetID);
			if (Success(db.DeleteAsset(assetID)))
				count++;
		}
		return count;
	}

	bool AssetManager::DeleteAsset_Internal(const fig::uuid& assetID) noexcept
	{
		if (DeleteAssetFile(assetID))
		{
			auto& db = GetDatabase();
			db.DeleteAsset(assetID);
			return false;
		}
		return false;
	}

	bool AssetManager::DeleteAssetFile(const fig::uuid& assetID) noexcept
	{
		auto itAsset = _assets.find(assetID);
		if (itAsset == _assets.end())
			return false;

		auto& asset = itAsset->second;
		auto path = _profilePath / asset.GetFileName();
		if (std::filesystem::exists(path) and std::filesystem::is_regular_file(path))
		{
			std::error_code err;
			if (std::filesystem::remove(path, err))
			{
				_assets.erase(itAsset);
				return true;
			}
		}
		return false;
	}

	bool AssetManager::ReleaseAssetData(const fig::uuid& assetID) noexcept
	{
		std::scoped_lock lock { _assetsMutex };
		auto itAsset = _assets.find(assetID);
		if (itAsset == _assets.end())
			return false;

		auto& asset = itAsset->second;
		asset.data.clear();
		asset.sync_state.has_data = false;
		asset.sync_state.file_sync = AssetSyncState::Status::Indeterminate;
		return true;
	}

	std::set<fig::uuid> AssetManager::FindRelatedAssets(const fig::uuid& assetID) noexcept
	{
		std::set<fig::uuid> assetIDs;
		std::set<fig::uuid> openList;

		// Find all related assets
		openList.insert(assetID);

		while (not openList.empty())
		{
			fig::uuid id = *openList.begin();
			openList.erase(id);
			auto itAsset = _assets.find(id);
			if (itAsset == _assets.end())
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
				if (auto import = ImportCharacter_Internal(filename, format))
					imported.push_back(std::ref(import.value()));
			}
		}

		return imported;
	}

	fig::expected_ref<Asset, FileError> AssetManager::ImportCharacter(const fig::path& filename, CharacterDataFormat format)
	{
		std::scoped_lock lock { _assetsMutex };
		return ImportCharacter_Internal(filename, format);
	}

	fig::expected_ref<Asset, FileError> AssetManager::ImportCharacter_Internal(const fig::path& filename, CharacterDataFormat format)
	{
		if (auto try_character = LoadCharacterData(filename, format))
		{
			auto& character = try_character.value();

			// Create asset
			fig::bytes characterData;
			character.SaveToXml(characterData);
			auto& characterAsset = CreateAsset_Internal(AssetType::Character, DataFormat::DataXml, characterData, _profileID);

			// Load portrait image(s)
			if (auto file = fig::io::ReadFile(filename))
			{
				// Create portrait asset
				auto& portraitAsset = CreateImageAsset_Internal(ImageType::LargePortrait, DataFormat::ImagePng, std::move(file.value()), characterAsset.id);

				// Create cover card
				if (auto coverImage = LoadImage(filename) //! @todo: load only once
					.transform([](auto img) {
					return CreateCoverImage(img, false);
				}))
				{
					// Save cover asset (bitmap)
					auto& coverAsset = CreateImageAsset_Internal(ImageType::CoverImage, coverImage.value(), characterAsset.id);
					coverAsset.SetMeta(MetaTag::ReferenceToOriginal, portraitAsset.id);
				}

				// Create square portrait
				if (auto squarePortraitImage = LoadImage(filename) //! @todo: load only once
					.transform([](auto img) {
					return CreateSquarePortrait(img);
				}))
				{
					// Save square portrait asset (bitmap)
					auto& squarePortraitAsset = CreateImageAsset_Internal(ImageType::SmallPortrait, squarePortraitImage.value(), characterAsset.id);
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
		return ImportScenario_Internal(filename);
	}

	fig::expected_ref<Asset, FileError> AssetManager::ImportScenario_Internal(const fig::path& filename)
	{
		Scenario scenario;
		if (auto error = scenario.LoadFromXml(filename); error != FileError::NoError)
			return std::unexpected(error);

		// Create asset
		fig::bytes scenarioData;
		scenario.SaveToXml(scenarioData);
		auto& scenarioAsset = CreateAsset_Internal(AssetType::Scenario, DataFormat::DataXml, scenarioData, _profileID);

		// Load scenario image
/*		if (not empty_or_whitespace(scenario.imageFilename))
		{
			if (auto file = fig::io::ReadFile(filename.parent_path() / scenario.imageFilename))
			{
				// Create portrait asset
				auto& scenarioImageAsset = CreateImageAsset_Internal(ImageType::Undefined, DataFormatFromExt(GetFileExt(scenario.imageFilename)), std::move(file.value()), scenarioAsset.id);

				// Create cover card
				if (auto coverImage = LoadImage(filename.parent_path() / scenario.imageFilename)
					.transform([](auto img) {
					return CreateCoverImage(img, false);
				}))
				{
					// Save cover asset (bitmap)
					auto& coverAsset = CreateImageAsset_Internal(ImageType::CoverImage, coverImage.value(), scenarioAsset.id);
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
				.asset_type { static_cast<uint8_t>(AssetType::Image) },
				.asset_subtype { static_cast<uint8_t>(ImageType::ProfileImage) },
				.data_format { static_cast<uint8_t>(DataFormat::ImageUncompressed) },
				.data_length { data.size() },
				.data_encrypted { false },
				.data { std::move(data) },
			};
			auto now = utc_now();
			image_file.meta[MetaTag::CreatedAt] = now;
			image_file.meta[MetaTag::UpdatedAt] = now;
			image_file.meta[MetaTag::LastUsedAt] = now;
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

	AssetDatabase& AssetManager::GetDatabase() noexcept
	{
		if (!_pAssetDB)
			_pAssetDB = std::make_unique<AssetDatabase>(_profilePath / fig::path(std::format("{}.{}", Constants::Paths::AssetIndexFileName, Constants::Paths::AssetIndexFileExt)));
		return *_pAssetDB.get();
	}

	static bool CreateSurface(const Asset& cover, fig::sdl::Surface& out_surface)
	{
		// Create SDL surface
		int32_t width = cover.GetMeta<uint16_t>(MetaTag::ImageWidth).value_or(Constants::GUI::CardWidth);
		int32_t height = cover.GetMeta<uint16_t>(MetaTag::ImageHeight).value_or(Constants::GUI::CardHeight);
		fig::gui::ImageFormat format = static_cast<ImageFormat>(cover.GetMeta<uint8_t>(MetaTag::ImageFormat).value_or(0));

		try
		{
			SurfacePtr pSurface = SDL_CreateSurface(width, height, to_sdl_format(format));
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

	AsyncLoadError AssetManager::__LoadImageTask(const fig::uuid& characterAssetID, ImageType imageType, AsyncResultVariant& outResult) noexcept
	{
		if (auto findImage = FindImageAsset(characterAssetID, imageType))
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

		if (auto findCover = FindImageAsset(characterAssetID, ImageType::CoverImage))
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
				if (auto findPortrait = FindImageAsset(characterAssetID, ImageType::LargePortrait))
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
								auto& coverAsset = CreateImageAsset_Internal(ImageType::CoverImage, coverImage, characterAssetID);
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
					error = __LoadImageTask(request.assetId, ImageType::LargePortrait, result);
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

	void AssetManager::ModifyAsset_Void(const fig::uuid& assetID, std::function<void(Asset&)> fn)
	{
		if (auto itFind = _assets.find(assetID); itFind != _assets.cend())
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

	bool AssetManager::ModifyAsset_Bool(const fig::uuid& assetID, std::function<bool(Asset&)> fn)
	{
		if (auto itFind = _assets.find(assetID); itFind != _assets.cend())
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
}