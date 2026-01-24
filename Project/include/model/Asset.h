#ifndef ASSET_H__
#define ASSET_H__
#pragma once

#include "Types.h"
#include "util/Serialization.h"
#include <map>
#include <variant>

namespace fig::fs
{
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

	enum class ImageSubtype : uint8_t
	{
		Generic			= 0,
		Cover			= 1,
		SquarePortrait	= 2,
		LargePortrait	= 3,
		Background		= 4,
	};

	class Asset
	{
	public:
		void SetData(fig::bytes&& data);
		void SetData(fig::byte_span data);
		constexpr fig::string AsString() const;

		fig::uuid id {};
		fig::uuid parentID {};
		DataFormat file_type { DataFormat::Undefined };
		AssetType asset_type { AssetType::Undefined };
		uint8_t asset_subtype {};

		fig::bytes data {};
		std::map<MetaTag, MetaValue> parameters {};
		bool needSave = false;

		AssetFile ToFile() const noexcept;
		void FromFile(const AssetFile& file) const;
	};
}

#endif