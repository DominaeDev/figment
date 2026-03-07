#include <pch.h>
#include <filesystem>
#include <format>
#include <ranges>
#include <set>
#include "model/AssetManager.h"
#include "model/UserManager.h"
#include "model/AppState.h"
#include "model/CharacterData.h"
#include "model/ScenarioData.h"
#include "util/Common.h"
#include "fs/Xml.h"
#include "fs/Serialization.h"
#include "fs/FileUtility.h"
#include "fs/CardImporter.h"
#include <cassert>

using namespace fig::io::data;
using namespace fig::util;
using namespace fig::gui::util;

namespace fig::io
{
	AssetManager::AssetManager(const fig::user::UserManager& userMngr, int32_t worker_threads)
	{
		auto const& profile = userMngr.GetActiveProfile();
		_profileAuthKey = userMngr.GetActiveAuthKey();
		_profileID = profile.id;
		_profilePath = profile.GetPath();

		if (LoadAssetIndex())
			LoadAssetMetaData();
		else
			Log(std::format("No asset index found for profile '{}'.", profile.name));

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
				request.promise->set_value(std::unexpected(FileError::Canceled));
			}
		}

		for (auto& worker : _workers)
			worker.request_stop();

		_pending_cv.notify_all();
	}

	bool AssetManager::LoadAssetIndex()
	{
		auto& db = GetDatabase();
		if (auto assets = db.FetchAssets(); assets.has_value())
		{
			std::scoped_lock lock { _assetsMutex };
			_assets = std::move(assets.value());
			return true;
		}
		else
			return false;
	}

	const Asset& AssetManager::CreateEmptyAsset(AssetType type, const fig::uuid& parent) noexcept
	{
		std::scoped_lock lock { _assetsMutex };

		return CreateEmptyAsset_Internal(type, parent);
	}

	Asset& AssetManager::CreateEmptyAsset_Internal(AssetType type, const fig::uuid& parent) noexcept
	{
		fig::uuid id = NewUUID();
		auto& asset = _assets[id] = Asset {};
		asset.id = id;
		asset.parent_id = not parent.empty() ? parent : _profileID;
		asset.asset_type = type;
		asset.file_status = AssetFileStatus::PartiallyLoaded;
		asset.SetMeta(MetaTag::CreatedAt, util::utc_now());
		asset.SetMeta(MetaTag::UpdatedAt, util::utc_now());
		return asset;
	}
	
	const Asset& AssetManager::CreateAsset(AssetType type, DataFormat format, fig::bytes&& data, const fig::uuid& parent) noexcept
	{
		std::scoped_lock lock { _assetsMutex };

		return CreateAsset_Internal(type, format, std::move(data), parent);
	}

	Asset& AssetManager::CreateAsset_Internal(AssetType type, DataFormat format, fig::bytes&& data, const fig::uuid& parent) noexcept
	{
		fig::uuid id = NewUUID();
		auto& asset = _assets[id] = Asset {};
		asset.id = id;
		asset.parent_id = not parent.empty() ? parent : _profileID;
		asset.asset_type = type;
		asset.data = std::move(data); // Move data
		asset.data_format = format;
		asset.file_status = AssetFileStatus::PartiallyLoaded;
		asset.SetMeta(MetaTag::CreatedAt, util::utc_now());
		asset.SetMeta(MetaTag::UpdatedAt, util::utc_now());
		return asset;
	}

	const Asset& AssetManager::CreateAsset(AssetType type, DataFormat format, fig::byte_span data, const fig::uuid& parent) noexcept
	{
		std::scoped_lock lock { _assetsMutex };

		return CreateAsset_Internal(type, format, data, parent);
	}

	Asset& AssetManager::CreateAsset_Internal(AssetType type, DataFormat format, fig::byte_span data, const fig::uuid& parent) noexcept
	{
		fig::uuid id = NewUUID();
		auto& asset = _assets[id] = Asset {};
		asset.id = id;
		asset.parent_id = not parent.empty() ? parent : _profileID;
		asset.asset_type = type;
		asset.data_format = format;
		asset.file_status = AssetFileStatus::PartiallyLoaded;
		asset.SetMeta(MetaTag::CreatedAt, util::utc_now());
		asset.SetMeta(MetaTag::UpdatedAt, util::utc_now());

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
		asset.file_status = AssetFileStatus::PartiallyLoaded;
		asset.data_format = DataFormat::ImageUncompressed;

		auto pSurface = surface.get();
		int32_t stride = pSurface->pitch / pSurface->w;

		asset.SetMeta(MetaTag::ImageWidth, static_cast<uint16_t>(pSurface->w));
		asset.SetMeta(MetaTag::ImageHeight, static_cast<uint16_t>(pSurface->h));
		asset.SetMeta(MetaTag::ImageFormatDepth, static_cast<uint8_t>(stride));

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

	std::optional<AssetRef> AssetManager::FindAsset(const fig::uuid& id) noexcept
	{
		std::scoped_lock lock { _assetsMutex };

		auto itFind = _assets.find(id);
		if (itFind != _assets.cend())
			return std::make_optional<AssetRef>(static_cast<Asset&>(std::ref(itFind->second)));
		return std::nullopt;
	}

	std::optional<AssetRef> AssetManager::FindAsset(const fig::uuid& parentId, ImageType imageType) noexcept
	{
		std::scoped_lock lock { _assetsMutex };

		auto itFind = std::find_if(_assets.begin(), _assets.end(),
			[&parentId, imageType](auto& kvp) {
				return kvp.second.parent_id == parentId
					and kvp.second.asset_type == AssetType::Image 
					and kvp.second.asset_subtype == static_cast<uint8_t>(imageType);
			});

		if (itFind != _assets.cend())
			return std::make_optional<AssetRef>(static_cast<Asset&>(std::ref(itFind->second)));
		return std::nullopt;
	}

	void AssetManager::SaveModified()
	{
		std::scoped_lock lock { _assetsMutex };

		for (auto& kvp : _assets)
		{
			auto& asset = kvp.second;
			if (asset.file_status == AssetFileStatus::Modified)
				WriteAsset(asset);
			if (asset.save_status != AssetSaveStatus::Saved)
				UpdateAsset(asset);
		}
	}

	bool AssetManager::WriteAsset(Asset& asset)
	{
		auto file = asset.ToFile();
		BinaryWriter writer(_profilePath, _profileAuthKey);
		if (auto error = writer.WriteFile(file); error == FileError::NoError)
		{
			asset.file_status = AssetFileStatus::FullyLoaded;
			return true;
		}
		return false;
	}

	bool AssetManager::UpdateAsset(Asset& asset)
	{
		auto& db = GetDatabase();
		if (asset.save_status == AssetSaveStatus::Created)
		{
			if (db.CreateAsset(asset) == DatabaseError::NoError)
			{
				asset.save_status = AssetSaveStatus::Saved;
				return true;
			}
		}
		else if (asset.save_status == AssetSaveStatus::Updated)
		{
			if (db.UpdateAsset(asset) == DatabaseError::NoError)
			{
				asset.save_status = AssetSaveStatus::Saved;
				return true;
			}
		}
		return false;
	}

	bool AssetManager::LoadAssetMetaData()
	{
		std::scoped_lock lock { _assetsMutex };

		for (auto& kvp : _assets)
		{
			auto& asset = kvp.second;
			if (asset.file_status > AssetFileStatus::NotLoaded)
				continue;

			BinaryReader reader(_profilePath, _profileAuthKey);
			if (auto file = reader.ReadFile(asset.GetFileName(), false))
			{
				asset.FromFile(std::move(file.value()));
				asset.file_status = AssetFileStatus::PartiallyLoaded;
			}
			else if (file.error() == FileError::FileNotFound)
				asset.file_status = AssetFileStatus::Missing;
			else
				asset.file_status = AssetFileStatus::Invalid; // Failed to load for some reason, but the file exists.
		}

		// Remove missing assets from index
		for (auto& id : _assets
			| std::views::filter([](auto& kvp) { return kvp.second.file_status == AssetFileStatus::Missing; })
			| std::views::keys
			| std::ranges::to<std::vector>())
			_assets.erase(id);

		return true;
	}

	FileError AssetManager::LoadAsset(const Asset& asset) noexcept
	{
		if (const auto result = LoadAsset(asset.id); result.has_value())
			return FileError::NoError;
		else
			return result.error();
	}

	std::expected<AssetRef, FileError> AssetManager::LoadAsset(const fig::uuid& id) noexcept
	{
		std::scoped_lock lock { _assetsMutex };
	
		auto itFind = _assets.find(id);
		if (itFind == _assets.cend())
			return std::unexpected(FileError::FileNotFound);
		
		Asset& asset = itFind->second;
		if (asset.file_status == AssetFileStatus::FullyLoaded)
			return asset;

		if (asset.file_status == AssetFileStatus::Invalid)
			return std::unexpected(FileError::ReadError);

		BinaryReader reader(_profilePath, _profileAuthKey);
		if (auto file = reader.ReadFile(asset.GetFileName()))
		{
			asset.FromFile(std::move(file.value()));
			asset.file_status = AssetFileStatus::FullyLoaded;
			return asset;
		}
		else
		{
			asset.file_status = AssetFileStatus::Invalid;
			return std::unexpected(FileError::ReadError);
		}
	}

	std::expected<CharacterData, FileError> AssetManager::ImportCharacter(fig::path filename, CharacterDataFormat format)
	{
		switch (format)
		{
		case CharacterDataFormat::Default:
		{
			CharacterData character;
			if (character.LoadFromXml(filename))
				return character;
		} break;
		case CharacterDataFormat::TavernV2:
			if (auto import = CardImporter::Import(filename); import.has_value())
				return import.value();
			break;
		default:
			return std::unexpected(FileError::UnrecognizedFormat);
		}		
		return std::unexpected(FileError::UnrecognizedFormat);
	}

	std::expected<AssetRef, FileError> AssetManager::ImportScenario(fig::path filename)
	{
		ScenarioData scenario;
		if (not scenario.LoadFromXml(filename))
			return std::unexpected(FileError::UnrecognizedFormat);

		fig::bytes scenarioData;
		scenario.SaveToXml(scenarioData);

		{
			std::scoped_lock lock { _assetsMutex };

			auto& scenarioAsset = CreateAsset_Internal(AssetType::Scenario, DataFormat::DataXml, scenarioData, _profileID);

			// Load scenario image
			if (not empty_or_whitespace(scenario.imageFilename))
			{
				if (auto file = fig::io::ReadFile(filename.parent_path() / scenario.imageFilename))
				{
					// Create portrait asset
					auto& scenarioImageAsset = CreateImageAsset_Internal(ImageType::Unspecified, DataFormatFromExt(GetFileExt(scenario.imageFilename)), std::move(file.value()), scenarioAsset.id);

					// Create cover card
					if (auto coverImage = LoadImage(filename.parent_path() / scenario.imageFilename)
						.transform([](auto img) {
						return CreateCoverImage(img);
					}))
					{
						// Save cover asset (bitmap)
						auto& coverAsset = CreateImageAsset_Internal(ImageType::CoverImage, coverImage.value(), scenarioAsset.id);
						coverAsset.SetMeta(MetaTag::ReferenceToOriginal, scenarioImageAsset.id);
					}
				}
			}
			return scenarioAsset;
		}

	}

	uint32_t AssetManager::DeleteAssets(std::span<fig::uuid> assetIDs) noexcept
	{
		std::scoped_lock lock { _assetsMutex };
	
		uint32_t count = 0;
		for (auto& id : assetIDs)
			count += DeleteAsset_Internal(id);
		return count;
	}

	uint32_t AssetManager::DeleteAsset(const fig::uuid& assetID) noexcept
	{
		std::set<fig::uuid> assetIDs;
		std::set<fig::uuid> openList;

		{
			std::scoped_lock lock { _assetsMutex };

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
		}

		auto& db = GetDatabase();

		// Delete
		uint32_t count = 0;
		for (auto& id : assetIDs)
		{
			if (DeleteAsset_Internal(id))
			{
				db.DeleteAsset(id);
				++count;
			}
		}
		return count;
	}

	bool AssetManager::DeleteAsset_Internal(const fig::uuid& assetID) noexcept
	{
		// Find asset and all related assets
		std::vector<fig::uuid> assetIds;

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

	void AssetManager::ImportCharactersInDirectory(fig::path directory, CharacterDataFormat format, size_t max_count)
	{
		std::vector<fig::path> files;
		for (const auto& entry : std::filesystem::directory_iterator(directory))
			files.push_back(entry.path());

		auto rng = std::random_device {};
		std::ranges::shuffle(files, rng);

		if (max_count > 0)
			files.resize(std::min(max_count, files.size()));

		{
			std::scoped_lock lock { _assetsMutex };
			for (auto& filename : files)
			{
				auto try_character = ImportCharacter(filename, format);
				if (not try_character.has_value())
					continue;

				auto& character = try_character.value();

				fig::bytes characterData;
				character.SaveToXml(characterData);
				auto& characterAsset = CreateAsset_Internal(AssetType::Character, DataFormat::DataXml, characterData, _profileID);

				// Load portrait image(s)
				if (auto file = fig::io::ReadFile(filename))
				{
					// Create portrait asset
					auto& portraitAsset = CreateImageAsset_Internal(ImageType::LargePortrait, DataFormat::ImagePng, std::move(file.value()), characterAsset.id);

					// Create cover card
					if (auto coverImage = LoadImage(filename)
						.transform([](auto img) {
						return CreateCoverImage(img);
					}))
					{
						// Save cover asset (bitmap)
						auto& coverAsset = CreateImageAsset_Internal(ImageType::CoverImage, coverImage.value(), characterAsset.id);
						coverAsset.SetMeta(MetaTag::ReferenceToOriginal, portraitAsset.id);
					}
				}
			}
		}
	}

	fig::uuid AssetManager::NewUUID() const noexcept
	{
		auto id = CreateUUID();
		while (_assets.contains(id))
			id = CreateUUID();
		return id;
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
		int32_t width = cover.GetMeta<uint16_t>(MetaTag::ImageWidth).value_or(Constants::GUI::HomeScreen::CardWidth);
		int32_t height = cover.GetMeta<uint16_t>(MetaTag::ImageHeight).value_or(Constants::GUI::HomeScreen::CardHeight);
		int32_t depth = cover.GetMeta<uint8_t>(MetaTag::ImageFormatDepth).value_or(4);

		try
		{
			fig::gui::SurfacePtr pSurface = SDL_CreateSurface(width, height, depth == 3 ? SDL_PIXELFORMAT_RGB24 : SDL_PIXELFORMAT_RGBA8888);
			if (!pSurface)
				return false;

			if (SDL_LockSurface(pSurface))
			{
				assert(pSurface->pitch * pSurface->h == cover.data.size());
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

	bool AssetManager::__LoadCoverImageTask(const fig::uuid& characterAssetID, fig::sdl::Surface& outSurface)
	{
		if (auto findCover = FindAsset(characterAssetID, ImageType::CoverImage))
		{
			Asset& cover = findCover.value();
			if (LoadAsset(cover) == FileError::NoError)
			{
				fig::sdl::Surface surface;
				if (CreateSurface(cover, surface))
				{
					outSurface = std::move(surface);
					return true;
				}
			}
		}

		// Create cover from portrait
		if (auto findPortrait = FindAsset(characterAssetID, ImageType::LargePortrait))
		{
			Asset& portraitAsset = findPortrait.value();
			if (LoadAsset(portraitAsset) == FileError::NoError)
			{
				if (auto portraitImage = LoadImageFromMemory(portraitAsset.data))
				{
					auto coverImage = ScaleSurface(portraitImage, Constants::GUI::HomeScreen::CardWidth, Constants::GUI::HomeScreen::CardHeight, ImageFit::Portrait);

					// Round corners
					MaskCorners(coverImage, CornerStyle::Card);

					// Save cover asset (bitmap)
					auto& coverAsset = CreateImageAsset_Internal(ImageType::CoverImage, coverImage, characterAssetID);
					coverAsset.SetMeta(MetaTag::Version, uint8_t { 1 });
					coverAsset.SetMeta(MetaTag::ReferenceToOriginal, portraitAsset.id);

					outSurface = std::move(coverImage);
					return true;
				}
			}
		}
		return false;
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

			if (not IsRequestAlive(request.assetId, request.promise.get()))
			{
				request.promise->set_value(std::unexpected(FileError::Canceled));
				continue;
			}

			// Do work
			bool bResult = false;
			fig::sdl::Surface surface;
			switch (request.task)
			{
			case AsyncLoad::Task::LoadImage:
			{
				bResult = __LoadCoverImageTask(request.assetId, surface);
			} break;
			}

			if (not IsRequestAlive(request.assetId, request.promise.get()))
			{
				request.promise->set_value(std::unexpected(FileError::Canceled));
				continue;
			}

			{	
				std::scoped_lock<std::mutex> lock(_active_mutex);
				_active_promises.erase(request.assetId);
			}

			// Fulfill promise
			if (bResult)
				request.promise->set_value(std::move(surface));
			else
				request.promise->set_value(std::unexpected(FileError::Canceled));
		}
	}

	[[nodiscard]] AssetManager::AsyncLoad AssetManager::LoadAssetAsync(const fig::uuid& assetId, AsyncLoad::Task task, int32_t priority)
	{
		const uint64_t id = _next_id.fetch_add(1, std::memory_order_relaxed);

		// Cancel the previous request for this card, if any
		{
			std::scoped_lock lock(_active_mutex);
			if (auto it = _active_promises.find(assetId); it != _active_promises.end())
			{
				it->second->set_value(std::unexpected(FileError::Canceled));
				_active_promises.erase(it);
			}
		}

		// Create the promise
		auto promise = std::make_unique<ImagePromise>();
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
			it->second->set_value(std::unexpected(FileError::Canceled));
			_active_promises.erase(it);
		}
	}

	void AssetManager::CancelAll()
	{
        {
            std::scoped_lock lock(_pending_mutex);
            while (!_pending.empty())
			{
				PendingRequest request = std::move(const_cast<PendingRequest&>(_pending.top()));
				_pending.pop();
                request.promise->set_value(std::unexpected(FileError::Canceled));
            }
        }
        _pending_cv.notify_all();
	}

	bool AssetManager::IsRequestAlive(const fig::uuid& assetId, const ImagePromise* p) const
	{
		std::scoped_lock lock(_active_mutex);
		auto it = _active_promises.find(assetId);
		if (it == _active_promises.cend())
			return false;
		return it != _active_promises.end() && it->second == p;
	}

}