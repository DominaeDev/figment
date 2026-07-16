#pragma once

#include "data/Character.h"
#include "data/Scenario.h"
#include "data/ChatInstance.h"
#include "data/ChatLog.h"
#include "gui/GUITypes.h"
#include "io/Asset.h"

namespace fig::io
{
	template <typename TAsset>
	inline constexpr AssetType AssetTypeOf = []<bool Flag = false>()
	{
		static_assert(Flag, "No AssetType mapping for this type");
	}();

	template <> 
	constexpr AssetType AssetTypeOf<fig::data::Character> = AssetType::Character;
	template <> 
	constexpr AssetType AssetTypeOf<fig::data::Scenario> = AssetType::Scenario;
	template <> 
	constexpr AssetType AssetTypeOf<fig::sdl::Surface> = AssetType::Image;
	template <> 
	constexpr AssetType AssetTypeOf<fig::data::ChatInstance> = AssetType::ChatInstance;
	template <> 
	constexpr AssetType AssetTypeOf<fig::data::ChatLog> = AssetType::ChatLog;

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