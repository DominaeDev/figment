#include <pch.h>
#include "io/ContentTypes.h"

using namespace fig::gui;
using namespace fig::data;

namespace fig::io
{
	std::expected<Character, FileError> AssetLoader<Character>::Load(const Asset& asset)
	{
		Character character;
		if (character.LoadFromXml(asset.AsStringView()) == FileError::NoError)
			return character;
		return std::unexpected(FileError::ReadError);
	}

	std::expected<Scenario, FileError> AssetLoader<Scenario>::Load(const Asset& asset)
	{
		Scenario scenario;
		if (scenario.LoadFromXml(asset.AsStringView()) == FileError::NoError)
			return scenario;
		return std::unexpected(FileError::ReadError);
	}

	std::expected<ChatInstance, FileError> AssetLoader<ChatInstance>::Load(const Asset& asset)
	{
		ChatInstance chatInstance;
		if (chatInstance.LoadFromXml(asset.AsStringView()) == FileError::NoError)
			return chatInstance;
		return std::unexpected(FileError::ReadError);
	}

	std::expected<ChatLog, FileError> AssetLoader<ChatLog>::Load(const Asset& asset)
	{
		ChatLog chatLog;
		if (chatLog.LoadFromXml(asset.AsStringView()) == FileError::NoError)
			return chatLog;
		return std::unexpected(FileError::ReadError);
	}

	std::expected<fig::sdl::Surface, FileError> AssetLoader<fig::sdl::Surface>::Load(const Asset& asset)
	{
		if (asset.data_format == DataFormat::ImageUncompressed)
		{
			int32_t width = asset.GetMeta<uint16_t>(MetaTag::ImageWidth).value_or(0);
			int32_t height = asset.GetMeta<uint16_t>(MetaTag::ImageHeight).value_or(0);
			ImageFormat format = static_cast<ImageFormat>(asset.GetMeta<uint8_t>(MetaTag::ImageFormat).value_or(0));
			if (width <= 0 or height <= 0 or format == ImageFormat::Undefined)
				return unexpected(FileError::UnrecognizedFormat);

			if (auto surface = fig::gui::CreateSurfaceFromBytes(width, height, format, asset.data); not surface.empty())
				return surface;
			return unexpected(FileError::ReadError);
		}
		else
		{
			//! @todo: png, jpeg...
		}
		return unexpected(FileError::UnrecognizedFormat);
	}
}