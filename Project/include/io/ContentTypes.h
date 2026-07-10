#pragma once

#include "data/Character.h"
#include "data/Scenario.h"
#include "data/ChatInstance.h"
#include "data/ChatLog.h"

namespace fig::io
{
	template <> 
	constexpr AssetType AssetTypeOf<fig::data::Character> = AssetType::Character;
	template <> 
	constexpr AssetType AssetTypeOf<fig::data::Scenario> = AssetType::Scenario;
	template <> 
	constexpr AssetType AssetTypeOf<fig::data::ChatInstance> = AssetType::ChatInstance;
	template <> 
	constexpr AssetType AssetTypeOf<fig::data::ChatLog> = AssetType::ChatLog;

	template <>
	struct AssetLoader<fig::data::Character>
	{
		std::expected<fig::data::Character, FileError> Load(const Asset& asset);
	};

	template <>
	struct AssetLoader<fig::data::Scenario>
	{
		std::expected<fig::data::Scenario, FileError> Load(const Asset& asset);
	};

	template <>
	struct AssetLoader<fig::data::ChatInstance>
	{
		std::expected<fig::data::ChatInstance, FileError> Load(const Asset& asset);
	};

	template <>
	struct AssetLoader<fig::data::ChatLog>
	{
		std::expected<fig::data::ChatLog, FileError> Load(const Asset& asset);
	};
}