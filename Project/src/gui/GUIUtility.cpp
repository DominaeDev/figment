#include <pch.h>
#include <algorithm>
#include "gui/GUIUtility.h"
#include "gui/AppResources.h"
#include "io/FileUtility.h"

#include <SDL3_image/SDL_image.h>
#include <c_resource.h>
#include <tuple>
#include <cassert>

namespace fig::gui
{
	fig::sdl::Texture CreateTexture(RendererPtr pRenderer, SurfacePtr pSurface)
	{
		if (!pSurface)
			return {};

		auto pTexture = SDL_CreateTextureFromSurface(pRenderer, pSurface);
		return fig::sdl::Texture::from_ptr(pTexture);
	}

	fig::sdl::Texture CreateTexture(RendererPtr pRenderer, const fig::sdl::Surface& surface)
	{
		if (surface.empty())
			return {};

		auto pTexture = SDL_CreateTextureFromSurface(pRenderer, surface.get());
		return fig::sdl::Texture::from_ptr(pTexture);
	}

	std::optional<fig::sdl::Surface> LoadImageFromMemory(fig::byte_span data)
	{
		SDL_IOStream* io = SDL_IOFromConstMem(data.data(), data.size());
		if (!io)
			return std::nullopt;

		SDL_Surface* pSurface = IMG_Load_IO(io, true);
		if (!pSurface)
			return std::nullopt;

		return fig::sdl::Surface::from_ptr(pSurface);
	}

	std::optional<fig::sdl::Surface> LoadImage(fig::path filename)
	{
		try
		{
			auto pSurface = IMG_Load(filename.u8string().c_str());
			if (!pSurface)
				return std::nullopt;
			return fig::make_cresource<fig::sdl::Surface>(pSurface);
		}
		catch (...)
		{
			return {};
		}
	}

	fig::sdl::Surface LoadAndResizeImage(fig::path filename, int32_t width, int32_t height, ImageFit fit)
	{
		try
		{
			fig::sdl::Surface surface;
			surface.reset(IMG_Load(filename.u8string().c_str()));

			if (!surface)
				return {};

			auto scaled = ScaleSurface(surface, width, height, fit);
//			IMG_SavePNG(scaled.get(), "./resized.png");
			return scaled;
		}
		catch (...)
		{
			return {};
		}
	}

	fig::sdl::Surface ScaleSurface(const fig::sdl::Surface& surface, int32_t width, int32_t height, ImageFit fit, bool bLinear)
	{
		auto pImage = surface.get();
		if (pImage == nullptr || width <= 0 || height <= 0)
			return {};

		auto pSurface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA8888);
		
		fig::sdl::Surface pScaledSurface;
		pScaledSurface.reset(pSurface);

		if (fit == ImageFit::None)
		{
			fig::gui::Rect srcRect { 0, 0, pImage->w, pImage->h };
			fig::gui::Rect dstRect { 0, 0, width, height };
			SDL_BlitSurface(pImage, &srcRect, pSurface, &dstRect);

		}
		else if (fit == ImageFit::Stretch)
		{
			fig::gui::Rect srcRect { 0, 0, pImage->w, pImage->h };
			fig::gui::Rect dstRect { 0, 0, width, height };
			SDL_StretchSurface(pImage, &srcRect, pSurface, &dstRect, bLinear ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);
		}
		else if (fit == ImageFit::SquarePortrait)
		{
			if (pImage->w < 128 or pImage->h < 128)
			{
				fig::gui::Rect srcRect { 0, 0, pImage->w, pImage->h };
				fig::gui::Rect dstRect { 0, 0, width, height };
				SDL_StretchSurface(pImage, &srcRect, pSurface, &dstRect, bLinear ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);
			}
			else
			{
				constexpr float fContract = 0.05f;
				float size = std::max(toF(pImage->w) * (1.0f - 2.0f * fContract), 128.0f);
				float offset = std::min(toF(pImage->w) * fContract, std::max(toF(pImage->h) - toF(pImage->w) * fContract, 0.0f));
				fig::gui::Rect srcRect { toI(offset), toI(offset), toI(size), toI(size) };
				fig::gui::Rect dstRect { 0, 0, width, height };
				SDL_StretchSurface(pImage, &srcRect, pSurface, &dstRect, bLinear ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);
			}
		}
		else
		{
			float srcWidth = toF(pImage->w);
			float srcHeight = toF(pImage->h);

			float scale;
			if (fit == ImageFit::Inside)
				scale = std::min(toF(width) / srcWidth, toF(height) / srcHeight);
			else if (fit == ImageFit::Outside || fit == ImageFit::Portrait)
				scale = std::max(toF(width) / srcWidth, toF(height) / srcHeight);
			else
				scale = 1.0f;

			float newWidth = srcWidth * scale;
			float newHeight = srcHeight * scale;
			float newX = std::roundf(-0.5f * (newWidth - width));
			float newY = std::roundf(-0.5f * (newHeight - height));
			if (fit == ImageFit::Portrait)
				newY = 0;

			fig::gui::Rect srcRect { 0, 0, pImage->w, pImage->h };
			fig::gui::Rect dstRect { toI(newX), toI(newY), toI(newWidth), toI(newHeight) };

			if (fit != ImageFit::Inside)
			{
				srcRect.x = std::max(toI(-newX / scale), 0);
				srcRect.y = std::max(toI(-newY / scale), 0);
				srcRect.w = std::min(srcRect.w - srcRect.x, toI(toF(width) / scale));
				srcRect.h = std::min(srcRect.h - srcRect.y, toI(toF(height) / scale));
			}

			// clip src
			fig::gui::Rect tmp;
			fig::gui::Rect fullRect { 0, 0, pImage->w, pImage->h };
			if (SDL_GetRectIntersection(&srcRect, &fullRect, &tmp))
				srcRect = tmp;

			// clip dst
			fig::gui::Rect fixedRect { 0, 0, width, height };
			if (SDL_GetRectIntersection(&dstRect, &fixedRect, &tmp))
				dstRect = tmp;

			SDL_StretchSurface(pImage, &srcRect, pSurface, &dstRect, bLinear ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);
		}

		return pScaledSurface;
	}

	void AlphaToMask(fig::path filename)
	{
		auto pMask = IMG_Load(filename.u8string().c_str());
		if (not (bool)pMask)
			return;

		if (SDL_LockSurface(pMask))
		{
			auto pixels = pMask->pixels;
			fig::bytes data(pMask->w * pMask->h);
			for (size_t i = 0; i < toUZ(pMask->w * pMask->h); ++i)
				data.data()[i] = ((std::byte*)pixels)[i * (pMask->pitch / pMask->w)];
			fig::io::WriteFile("./mask.bin", data);
			SDL_UnlockSurface(pMask);
		}
	}

	bool MaskCorners(fig::sdl::Surface& surface, MaskType mask)
	{
		auto pImage = surface.get();
		if (pImage == nullptr || pImage->w <= 0 || pImage->h <= 0)
			return false;

		// Load mask
		auto pMask = AppResources::GetMask(mask);
		if (!pMask)
			return false;

		// Convert to RGBA
		if (pImage->format != SDL_PIXELFORMAT_RGBA8888)
		{
			if (auto newSurface = SDL_ConvertSurface(pImage, SDL_PIXELFORMAT_RGBA8888))
			{
				surface.reset(newSurface);
				pImage = newSurface;
			}
		}

		const uint8_t* mask_pixels = pMask->pixels.data();
		size_t mask_width = pMask->width;
		size_t mask_pitch = mask_width;
		size_t corner_size = mask_width / 2;

		if (SDL_LockSurface(pImage))
		{
			auto pixels = (uint8_t*)pImage->pixels;
			auto stride = pImage->pitch / pImage->w;
			
			const std::tuple<size_t, size_t, size_t, size_t> offsets[4] {
				/* Top left */		{ 0, 0, 0, 0 },
				/* Top right  */	{ corner_size, 0, pImage->w - corner_size, 0 },
				/* Bottom left */	{ 0, corner_size, 0, pImage->h - corner_size },
				/* Bottom right */	{ corner_size, corner_size, pImage->w - corner_size, pImage->h - corner_size },
			};

			for (auto& offset : offsets)
			{
				std::pair<size_t, size_t> msk_offset { std::get<0>(offset), std::get<1>(offset) };
				std::pair<size_t, size_t> img_offset { std::get<2>(offset), std::get<3>(offset) };
				
				for (int32_t y = 0; y < corner_size; ++y)
				{
					size_t msk_row = (y + msk_offset.second) * mask_pitch;
					size_t img_row = (y + img_offset.second) * pImage->pitch;
					for (size_t x = 0; x < corner_size; ++x)
					{
						auto& p = pixels[img_row + ((x + img_offset.first) * stride) + 0];
						auto& m = mask_pixels[msk_row + (x + msk_offset.first)];

						p = (uint8_t)((uint16_t)p * m / 255);
					}
				}
			}
			SDL_UnlockSurface(pImage);
		}

		return true;
	}

	fig::sdl::Surface CreateCoverImage(const fig::sdl::Surface& surface, bool bAlpha)
	{
		int32_t expandX = Constants::GUI::CardZoomPixelsLarge * 2;
		int32_t expandY = toI(std::ceilf(toF(Constants::GUI::CardZoomPixelsLarge * 2) * toF(Constants::GUI::CardHeight) / toF(Constants::GUI::CardWidth)));

		auto pSurface = SDL_CreateSurface(Constants::GUI::CardWidth + expandX, Constants::GUI::CardHeight + expandY, SDL_PIXELFORMAT_RGB24);
		if (not (bool)pSurface)
			return {};
		
		fig::sdl::Surface cover {};
		cover.reset(pSurface);

		// Draw background
		auto pBGImage = AppResources::GetImage(TextureType::CARD_BACKGROUND_DEFAULT);
		SDL_BlitSurface(pBGImage, NULL, pSurface, NULL);

		auto pScaledImage = ScaleSurface(surface, Constants::GUI::CardWidth + 36, Constants::GUI::CardHeight + 48, ImageFit::Portrait);
		SDL_BlitSurface(pScaledImage.get(), NULL, pSurface, NULL);

		if (bAlpha)
		{
			if (auto newSurface = SDL_ConvertSurface(pSurface, SDL_PIXELFORMAT_RGBA8888)) // Add alpha channel
			{
				SDL_DestroySurface(pSurface);
				cover.reset(newSurface);

				if constexpr (Disabled) 
				{
					MaskCorners(cover, MaskType::CARD_CORNER_MASK);
				}
			}
		}

		return cover;
	}

	fig::sdl::Surface CreateSquarePortrait(const fig::sdl::Surface& surface)
	{
		auto pSurface = SDL_CreateSurface(Constants::Data::SmallPortraitWidth, Constants::Data::SmallPortraitWidth, SDL_PIXELFORMAT_RGB24);
		if (not (bool)pSurface)
			return {};

		// Draw background
		auto pBGImage = AppResources::GetImage(TextureType::SQUARE_BACKGROUND_DEFAULT);
		SDL_BlitSurface(pBGImage, NULL, pSurface, NULL);

		auto pScaledImage = ScaleSurface(surface, pSurface->w, pSurface->w, ImageFit::SquarePortrait, true);
		SDL_BlitSurface(pScaledImage.get(), NULL, pSurface, NULL);
		return fig::sdl::Surface::from_ptr(pSurface);
	}

	fig::sdl::Surface CreateProfileImage(const fig::sdl::Surface& surface)
	{
		auto pSurface = SDL_CreateSurface(Constants::GUI::ProfileImageWidth, Constants::GUI::ProfileImageWidth, SDL_PIXELFORMAT_RGBA8888);
		if (not (bool)pSurface)
			return {};

		return ScaleSurface(surface, Constants::GUI::ProfileImageWidth, Constants::GUI::ProfileImageWidth, ImageFit::Portrait);
	}

	fig::sdl::Surface CreateSurfaceFromBytes(int16_t width, int16_t height, ImageFormat format, fig::byte_span data)
	{
		if (width <= 0 || height <= 0 || format == ImageFormat::Undefined)
			return {};

		int depth;
		switch (format)
		{
			case ImageFormat::RGB24: depth = 3; break;
			case ImageFormat::RGBA32: depth = 4; break;
			default:
				return {}; // Error
		}

		if (toUZ(width * height * depth) != data.size())
			return {}; // Invalid data length

		try
		{
			// Create SDL surface
			SurfacePtr pSurface = SDL_CreateSurface(width, height, to_sdl_format(format));
			if (!pSurface)
				return {};

			// Write pixel data
			if (SDL_LockSurface(pSurface))
			{
				std::memcpy(pSurface->pixels, data.data(), data.size());
				SDL_UnlockSurface(pSurface);

				return std::move(fig::sdl::Surface::from_ptr(pSurface));
			}
		}
		catch (...)
		{
		}
		return {};
	}

	Point MeasureText(Font& font, const fig::string& text)
	{
		int w, h;
		if (TTF_GetStringSize(&font, text.c_str(), 0, &w, &h))
			return Point { w, h };
		return {};
	}

	int MeasureFontHeight(Font& font)
	{
		return TTF_GetFontHeight(&font);
	}
}