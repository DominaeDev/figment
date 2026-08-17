#pragma once

#include "data/Character.h"
#include "data/Scenario.h"
#include "data/ChatInstance.h"
#include "data/ChatLog.h"
#include "data/VoiceSettings.h"
#include "gui/GUITypes.h"
#include "io/Asset.h"

namespace fig::io
{
	template <typename TAsset>
	inline constexpr AssetTypeDefinition AssetTypeOf = []<bool Flag = false>()
	{
		static_assert(Flag, "No AssetType mapping for this type");
	}();

	template <> 
	constexpr AssetTypeDefinition AssetTypeOf<fig::data::Character> = make_asset_type(AssetType::Character);
	template <> 
	constexpr AssetTypeDefinition AssetTypeOf<fig::data::Scenario> = make_asset_type(AssetType::Scenario);
	template <> 
	constexpr AssetTypeDefinition AssetTypeOf<fig::sdl::Surface> = make_asset_type(AssetType::Image);
	template <> 
	constexpr AssetTypeDefinition AssetTypeOf<fig::data::ChatInstance> = make_asset_type(AssetType::Chat, ChatAssetType::Instance);
	template <>
	constexpr AssetTypeDefinition AssetTypeOf<fig::data::ChatLog> = make_asset_type(AssetType::Chat, ChatAssetType::Log);
	template <>
	constexpr AssetTypeDefinition AssetTypeOf<fig::data::VoiceSettings> = make_asset_type(AssetType::Audio, AudioAssetType::VoiceSettings);

	template<typename T>
	concept XmlLoadableAsset = requires (T& value)
	{
		{ value.LoadFromXml(string_view {}) } -> std::same_as<FileError>;
	};

	template<typename T>
	struct AssetLoader 
	{
		std::expected<T, FileError> Load(const Asset& asset)
		{
			static_assert(false, "No asset loader implemented for this type");
		}
	};

	template <XmlLoadableAsset T>
	struct AssetLoader<T>
	{
		std::expected<T, FileError> Load(const Asset& asset)
		{
			T value;
			if (auto error = value.LoadFromXml(asset.AsStringView()); error == FileError::NoError)
				return value;
			else
				return std::unexpected(error);
		}
	};

	template <>
	struct AssetLoader<fig::sdl::Surface>
	{
		std::expected<fig::sdl::Surface, FileError> Load(const Asset& asset);
	};
}