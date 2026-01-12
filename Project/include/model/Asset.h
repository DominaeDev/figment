#ifndef ASSET_H__
#define ASSET_H__
#pragma once

#include "Types.h"
#include <map>
#include <variant>

namespace fig::fs
{
	enum class FileType : unsigned short
	{
		Undefined,
		TextGeneric,
		TextBase64,
		TextXml,
		TextJson,
		ImagePng,
		ImageJpeg,
	};

	enum class AssetType : unsigned short
	{
		Undefined,
		Character,
		Scenario,
		Concept,

		ImageGeneric,
		ImageCover,
		ImageCharacter,
		ImagePortrait,
		ImageBackground,
		
		ChatLog,
		ChatParameters,
	};

	using ParamVar = std::variant<int32_t, float, fig::string>;

	struct Asset
	{
		FileType file_type { FileType::Undefined };
		AssetType asset_type { AssetType::Undefined };
		fig::bytes data {};
		std::map<fig::string, ParamVar> parameters {};

		constexpr fig::string as_string() const;
	};
}

#endif