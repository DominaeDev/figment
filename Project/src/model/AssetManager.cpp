#include <pch.h>
#include "model/AssetManager.h"
#include "model/UserManager.h"
#include "model/AppState.h"
#include "util/Common.h"
#include "util/BinaryWriter.h"
#include "util/BinaryReader.h"
#include "util/Xml.h"
#include "util/FileUtility.h"
#include "model/Character.h"
#include <filesystem>
#include <format>
#include <ranges>

using namespace fig::data;
using namespace fig::common_util;
using namespace fig::string_util;
using namespace fig::gui_util;

namespace fig::fs
{
	AssetManager::AssetManager(const UserManager& userMngr)
	{
		auto const& profile = userMngr.GetActiveProfile();
		_profileAuthKey = userMngr.GetActiveAuthKey();
		_profileID = profile.id;
		_profilePath = profile.GetPath();

		if (not LoadIndex())
			Log(std::format("No asset index found for profile '{}'.", profile.name));

		LoadAssetMetaData();
	}

	bool AssetManager::LoadIndex()
	{
		auto const path = _profilePath / std::format("{}.{}", Constants::Paths::ProfileIndexFileName, Constants::Paths::ProfileIndexFileExt);

		fig::XmlReader xml(path.u8string(), "Assets");
		if (not xml.IsOk())
			return false; // Invalid document type

		_assets.clear();

		auto optAsset = xml.GetFirstElement("Asset");
		while (optAsset)
		{
			auto& assetNode = optAsset.value();

			Asset asset {};
			asset.id = assetNode.GetElementUUID("ID").value_or({});
			asset.parent_id = assetNode.GetElementUUID("ParentID").value_or({});
			auto [type, subtype] = AssetTypeFromString(assetNode.GetElementText("Type").value_or(""));
			asset.asset_type = type;
			asset.asset_subtype = subtype;
			asset.status = AssetFileStatus::NotLoaded;
			if (!asset.id.empty())
				_assets[asset.id] = asset;
			optAsset = assetNode.GetNextSibling();
		}

		return true;
	}

	bool AssetManager::SaveIndex() const
	{
		auto const path = _profilePath / std::format("{}.{}", Constants::Paths::ProfileIndexFileName, Constants::Paths::ProfileIndexFileExt);

		XmlWriter xml("Assets");
		for (auto& kvp : _assets)
		{
			auto& asset = kvp.second;
			auto assetNode = xml.AddChild("Asset");
			assetNode.SetElement("ID", asset.id);
			assetNode.SetElement("ParentID", asset.parent_id);
			assetNode.SetElement("Type", AssetTypeToString(asset.asset_type, asset.asset_subtype));
		}

		return xml.Save(path.u8string());
	}

	Asset& AssetManager::CreateEmptyAsset(AssetType type, const fig::uuid& parent) noexcept
	{
		fig::uuid id = CreateUUID();
		auto& newAsset = _assets[id] = Asset {};
		newAsset.id = id;
		newAsset.parent_id = not parent.empty() ? parent : _profileID;
		newAsset.asset_type = type;
		newAsset.status = AssetFileStatus::PartiallyLoaded;
		newAsset.SetMeta(MetaTag::CreatedAt, common_util::utc_now());
		newAsset.SetMeta(MetaTag::UpdatedAt, common_util::utc_now());
		return newAsset;
	}

	Asset& AssetManager::CreateAssetReference(ReferenceType refType, const fig::uuid& referenceId, const fig::uuid& parent) noexcept
	{
		fig::uuid id = CreateUUID();
		auto& newAsset = _assets[id] = Asset {};
		newAsset.id = id;
		newAsset.parent_id = not parent.empty() ? parent : _profileID;
		newAsset.asset_type = AssetType::Reference;
		newAsset.asset_subtype = static_cast<uint8_t>(refType);
		newAsset.status = AssetFileStatus::Modified;
		newAsset.SetMeta(MetaTag::CreatedAt, common_util::utc_now());
		newAsset.SetMeta(MetaTag::UpdatedAt, common_util::utc_now());
		newAsset.SetMeta(MetaTag::Reference, referenceId);
		return newAsset;
	}

	Asset& AssetManager::CreateAsset(AssetType type, fig::bytes&& data, const fig::uuid& parent) noexcept
	{
		fig::uuid id = CreateUUID();
		auto& newAsset = _assets[id] = Asset {};
		newAsset.id = id;
		newAsset.parent_id = not parent.empty() ? parent : _profileID;
		newAsset.asset_type = type;
		newAsset.data = std::move(data); // Move data
		newAsset.status = AssetFileStatus::Modified;
		newAsset.SetMeta(MetaTag::CreatedAt, common_util::utc_now());
		newAsset.SetMeta(MetaTag::UpdatedAt, common_util::utc_now());
		return newAsset;
	}

	Asset& AssetManager::CreateAsset(AssetType type, fig::byte_span data, const fig::uuid& parent) noexcept
	{
		fig::uuid id = CreateUUID();
		auto& newAsset = _assets[id] = Asset {};
		newAsset.id = id;
		newAsset.parent_id = not parent.empty() ? parent : _profileID;
		newAsset.asset_type = type;
		newAsset.status = AssetFileStatus::Modified;
		newAsset.SetMeta(MetaTag::CreatedAt, common_util::utc_now());
		newAsset.SetMeta(MetaTag::UpdatedAt, common_util::utc_now());

		// Copy data
		newAsset.data.resize(data.size());
		std::memcpy(newAsset.data.data(), data.data(), data.size());
		return newAsset;
	}

	Asset& AssetManager::CreateImageAsset(ImageType subtype, DataFormat format, fig::bytes&& data, const fig::uuid& parent) noexcept
	{
		auto& asset = CreateAsset(AssetType::Image, std::move(data), parent);
		asset.asset_subtype = static_cast<uint8_t>(subtype);
		asset.data_format = format;
		return asset;
	}

	Asset& AssetManager::CreateImageAsset(ImageType subtype, DataFormat format, fig::byte_span data, const fig::uuid& parent) noexcept
	{
		auto& asset = CreateAsset(AssetType::Image, data, parent);
		asset.asset_subtype = static_cast<uint8_t>(subtype);
		asset.data_format = format;
		return asset;
	}

	Asset& AssetManager::CreateImageAsset(ImageType subtype, const fig::sdl::Surface& surface, const fig::uuid& parent) noexcept
	{
		auto& asset = CreateEmptyAsset(AssetType::Image, parent);
		if (!surface.get())
			return asset; // Error

		asset.asset_subtype = static_cast<uint8_t>(subtype);
		asset.status = AssetFileStatus::Modified;

		auto pSurface = surface.get();
		int32_t stride = pSurface->pitch / pSurface->w;
		if (stride == 4)
			asset.data_format = DataFormat::ImageARGB32;
		else if (stride == 3)
			asset.data_format = DataFormat::ImageRGB24;
		else
			asset.data_format = DataFormat::Undefined;

		asset.SetMeta(MetaTag::ImageWidth, pSurface->w);
		asset.SetMeta(MetaTag::ImageHeight, pSurface->h);

		if (SDL_LockSurface(pSurface))
		{
			size_t data_length = toUZ(pSurface->h * pSurface->pitch);

			// Copy pixel data
			asset.data.resize(data_length);
			std::memcpy(asset.data.data(), (fig::byte*)pSurface->pixels, data_length);
			SDL_UnlockSurface(pSurface);
		}
		return asset;
	}

	std::optional<AssetRef> AssetManager::FindAsset(const fig::uuid& id) noexcept
	{
		auto itFind = _assets.find(id);
		if (itFind != _assets.cend())
			return std::make_optional<AssetRef>(static_cast<Asset&>(itFind->second));
		return std::nullopt;
	}

	void AssetManager::SaveModified()
	{
		for (auto& kvp : _assets)
		{
			auto& asset = kvp.second;
			if (asset.status != AssetFileStatus::Modified)
				continue;

			if (WriteAsset(asset))
				asset.status = AssetFileStatus::FullyLoaded;
		}
		SaveIndex();
	}

	bool AssetManager::WriteAsset(const Asset& asset) const
	{
		auto file = asset.ToFile();
		BinaryWriter writer(_profileAuthKey);
		auto error = writer.WriteFile(_profilePath, file);
		return error == FileError::NoError;
	}

	bool AssetManager::LoadAssetMetaData()
	{
		for (auto& kvp : _assets)
		{
			auto& asset = kvp.second;
			if (asset.status > AssetFileStatus::NotLoaded)
				continue;

			BinaryReader reader(_profilePath, _profileAuthKey);
			if (auto file = reader.ReadFile(asset.GetFileName(), false))
			{
				asset.FromFile(std::move(file.value()));
				asset.status = AssetFileStatus::PartiallyLoaded;
			}
			else if (file.error() == FileError::FileNotFound)
				asset.status = AssetFileStatus::Missing;
			else
				asset.status = AssetFileStatus::Invalid; // Failed to load for some reason, but the file exists.
		}

		// Remove missing assets from index
		for (auto& id : _assets
			| std::views::filter([](auto& kvp) { return kvp.second.status == AssetFileStatus::Missing; })
			| std::views::keys
			| std::ranges::to<std::vector>())
			_assets.erase(id);

		return true;
	}

	std::expected<AssetRef, FileError> AssetManager::LoadAsset(const fig::uuid& id) noexcept
	{
		auto findAsset = FindAsset(id);
		if (not findAsset.has_value())
			return std::unexpected(FileError::FileNotFound);
		
		Asset& asset = findAsset.value();
		if (asset.status == AssetFileStatus::FullyLoaded)
			return asset;

		if (asset.status == AssetFileStatus::Invalid)
			return std::unexpected(FileError::ReadError);

		BinaryReader reader(_profilePath, _profileAuthKey);
		if (auto file = reader.ReadFile(asset.GetFileName()))
		{
			asset.FromFile(std::move(file.value()));
			asset.status = AssetFileStatus::FullyLoaded;
			return asset;
		}
		else
		{
			asset.status = AssetFileStatus::Invalid;
			return std::unexpected(FileError::ReadError);
		}
	}

	std::expected<AssetRef, FileError> AssetManager::ImportCharacter(fig::path filename, CharacterDataFormat format)
	{
		CharacterData character;
		if (not character.LoadFromXml(filename.u8string()))
			return std::unexpected(FileError::UnrecognizedFormat);

		fig::bytes characterData;
		character.SaveToXml(characterData);
		auto& characterAsset = CreateAsset(AssetType::Character, characterData, _profileID);

		// Load portrait image(s)
		if (not empty_or_whitespace(character.largePortraitFilename))
		{
			if (auto file = fig::fs::ReadFile(filename.parent_path() / character.largePortraitFilename))
			{
				// Create portrait asset
				auto& portraitAsset = CreateImageAsset(ImageType::LargePortrait, DataFormatFromExt(GetFileExt(character.largePortraitFilename)), std::move(file.value()), characterAsset.id);

				// Create cover card
				if (auto coverImage = LoadAndResizeImage(filename.parent_path() / character.largePortraitFilename, Constants::GUI::CardWidth, Constants::GUI::CardHeight, ImageFit::Portrait))
				{
					// Round corners
					MaskCorners(coverImage, CornerStyle::Card);

					// Save cover asset (bitmap)
					auto& coverAsset = CreateImageAsset(ImageType::CoverImage, coverImage, characterAsset.id);

					// Create reference to original
					CreateAssetReference(ReferenceType::Original, portraitAsset.id, coverAsset.id);
				}
			}
		}

		if (not empty_or_whitespace(character.smallPortraitFilename))
		{
			if (auto file = fig::fs::ReadFile(filename.parent_path() / character.smallPortraitFilename))
			{
				CreateImageAsset(ImageType::SmallPortrait, DataFormatFromExt(GetFileExt(character.smallPortraitFilename)), std::move(file.value()), characterAsset.id);
			}
		}

		return characterAsset;
	}
}