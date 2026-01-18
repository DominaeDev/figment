#ifndef ASSET_H__
#define ASSET_H__
#pragma once

#include "Types.h"
#include <map>
#include <variant>

namespace fig::fs
{
	enum class DataFormat : uint8_t
	{
		Undefined = 0,
		Binary = 1,
		BinaryBase64 = 2,

		TextGeneric = 4,
		TextXml = 5,
		TextJson = 6,

		ImagePng = 8,
		ImageJpeg = 9,
	};

	enum class AssetType : uint8_t
	{
		Undefined = 0,

		Character = 1,
		Scenario = 2,
		Concept = 3,

		Image = 10,

		ChatSession = 20,
		ChatLog = 21,
	};

	enum class ImageSubtype : uint8_t
	{
		Generic			= 0,
		Cover			= 1,
		SquarePortrait	= 2,
		LargePortrait	= 3,
		Background		= 4,
	};

	using ParamVar = std::variant<int32_t, float, fig::string>;

	struct Asset
	{
		DataFormat file_type {};
		AssetType asset_type {};
		uint8_t asset_subtype {};

		fig::bytes data {};
		std::map<fig::string, ParamVar> parameters {};

		constexpr fig::string as_string() const;
	};
}

#endif