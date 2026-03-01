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

using namespace fig::io::data;
using namespace fig::util;
using namespace fig::gui::util;

namespace fig::io
{
	AssetManager::AssetManager(const fig::user::UserManager& userMngr)
	{
		auto const& profile = userMngr.GetActiveProfile();
		_profileAuthKey = userMngr.GetActiveAuthKey();
		_profileID = profile.id;
		_profilePath = profile.GetPath();

		if (LoadAssetIndex())
			LoadAssetMetaData();
		else
			Log(std::format("No asset index found for profile '{}'.", profile.name));
	}

	bool AssetManager::LoadAssetIndex()
	{
		auto& db = GetDatabase();
		if (auto assets = db.FetchAssets(); assets.has_value())
		{
			_assets = std::move(assets.value());
			return true;
		}
		else
			return false;
	}

	Asset& AssetManager::CreateEmptyAsset(AssetType type, const fig::uuid& parent) noexcept
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

	Asset& AssetManager::CreateAsset(AssetType type, DataFormat format, fig::bytes&& data, const fig::uuid& parent) noexcept
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

	Asset& AssetManager::CreateAsset(AssetType type, DataFormat format, fig::byte_span data, const fig::uuid& parent) noexcept
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

	Asset& AssetManager::CreateImageAsset(ImageType subtype, DataFormat format, fig::bytes&& data, const fig::uuid& parent) noexcept
	{
		auto& asset = CreateAsset(AssetType::Image, format, std::move(data), parent);
		asset.asset_subtype = static_cast<uint8_t>(subtype);
		asset.CalculateChecksum();
		return asset;
	}

	Asset& AssetManager::CreateImageAsset(ImageType subtype, DataFormat format, fig::byte_span data, const fig::uuid& parent) noexcept
	{
		auto& asset = CreateAsset(AssetType::Image, format, data, parent);
		asset.asset_subtype = static_cast<uint8_t>(subtype);
		asset.CalculateChecksum();
		return asset;
	}

	Asset& AssetManager::CreateImageAsset(ImageType subtype, const fig::sdl::Surface& surface, const fig::uuid& parent) noexcept
	{
		auto& asset = CreateEmptyAsset(AssetType::Image, parent);
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
		auto itFind = _assets.find(id);
		if (itFind != _assets.cend())
			return std::make_optional<AssetRef>(static_cast<Asset&>(std::ref(itFind->second)));
		return std::nullopt;
	}

	std::optional<AssetRef> AssetManager::FindAsset(const fig::uuid& parentId, ImageType imageType) noexcept
	{
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
		auto findAsset = FindAsset(id);
		if (not findAsset.has_value())
			return std::unexpected(FileError::FileNotFound);
		
		Asset& asset = findAsset.value();
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
		auto& scenarioAsset = CreateAsset(AssetType::Scenario, DataFormat::DataXml, scenarioData, _profileID);

		// Load scenario image
		if (not empty_or_whitespace(scenario.imageFilename))
		{
			if (auto file = fig::io::ReadFile(filename.parent_path() / scenario.imageFilename))
			{
				// Create portrait asset
				auto& scenarioImageAsset = CreateImageAsset(ImageType::Unspecified, DataFormatFromExt(GetFileExt(scenario.imageFilename)), std::move(file.value()), scenarioAsset.id);

				// Create cover card
				if (auto coverImage = LoadImage(filename.parent_path() / scenario.imageFilename)
					.transform([](auto img) {
						return CreateCoverImage(img);
					}))
				{
					// Save cover asset (bitmap)
					auto& coverAsset = CreateImageAsset(ImageType::CoverImage, coverImage.value(), scenarioAsset.id);
					coverAsset.SetMeta(MetaTag::ReferenceToOriginal, scenarioImageAsset.id);
				}
			}
		}

		return scenarioAsset;
	}

	uint32_t AssetManager::DeleteAssets(std::span<fig::uuid> assetIDs) noexcept
	{
		uint32_t count = 0;
		for (auto& id : assetIDs)
			count += DeleteAsset(id);
		return count;
	}

	uint32_t AssetManager::DeleteAsset(const fig::uuid& assetID) noexcept
	{
		// Find all related assets
		std::set<fig::uuid> assetIDs;
		std::set<fig::uuid> openList;
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

		auto& db = GetDatabase();

		// Delete
		uint32_t count = 0;
		for (auto& id : assetIDs)
		{
			if (EraseAsset(id))
			{
				db.DeleteAsset(id);
				++count;
			}
		}
		return count;
	}

	bool AssetManager::EraseAsset(const fig::uuid& assetID) noexcept
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

	void AssetManager::ImportCharacters(fig::path directory, size_t max_count)
	{
		std::vector<fig::path> files;
		for (const auto& entry : std::filesystem::directory_iterator(directory))
			files.push_back(entry.path());

		auto rng = std::random_device {};
		std::ranges::shuffle(files, rng);

		if (max_count > 0)
			files.resize(std::min(max_count, files.size()));

		for (auto& filename : files)
		{
			auto try_character = ImportCharacter(filename, fig::io::AssetManager::CharacterDataFormat::TavernV2);
			if (not try_character.has_value())
				continue;

			auto& character = try_character.value();

			fig::bytes characterData;
			character.SaveToXml(characterData);
			auto& characterAsset = CreateAsset(AssetType::Character, DataFormat::DataXml, characterData, _profileID);

			// Load portrait image(s)
			if (auto file = fig::io::ReadFile(filename))
			{
				// Create portrait asset
				auto& portraitAsset = CreateImageAsset(ImageType::LargePortrait, DataFormat::ImagePng, std::move(file.value()), characterAsset.id);

				// Create cover card
				if (auto coverImage = LoadImage(filename)
					.transform([](auto img) {
						return CreateCoverImage(img);
					}))
				{
					// Save cover asset (bitmap)
					auto& coverAsset = CreateImageAsset(ImageType::CoverImage, coverImage.value(), characterAsset.id);
					coverAsset.SetMeta(MetaTag::ReferenceToOriginal, portraitAsset.id);
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

}