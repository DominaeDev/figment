#include <pch.h>
#include <SDL3_image/SDL_image.h>
#include "io/ContentTypes.h"
#include "gui/GUIUtility.h"

using namespace fig::gui;
using namespace fig::data;

namespace fig::io
{
	std::expected<fig::sdl::Surface, FileError> AssetLoader<fig::sdl::Surface>::Load(const Asset& asset)
	{
		if (asset.data_format == DataFormat::ImageUncompressed)
		{
			int32_t width = asset.GetMeta<uint16_t>(MetaTag::ImageWidth).value_or(0);
			int32_t height = asset.GetMeta<uint16_t>(MetaTag::ImageHeight).value_or(0);
			ImageFormat format = static_cast<ImageFormat>(asset.GetMeta<uint8_t>(MetaTag::ImageFormat).value_or(0));
			if (width <= 0 or height <= 0 or format == ImageFormat::Undefined)
				return unexpected(FileError::UnrecognizedFormat);

			if (auto surface = CreateSurfaceFromBytes(width, height, format, asset.data); not surface.empty())
				return surface;
			return unexpected(FileError::ReadError);
		}
		else if (asset.data_format == DataFormat::ImagePng
			or asset.data_format == DataFormat::ImageJpeg
			or asset.data_format == DataFormat::ImageWebp)
		{
			if (auto try_load = LoadImageFromMemory(asset.data))
				return std::move(try_load.value());
			return unexpected(FileError::ReadError);
		}
		return unexpected(FileError::UnrecognizedFormat);
	}
}