#include <pch.h>
#include "io/ContentTypes.h"

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