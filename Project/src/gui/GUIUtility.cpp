#include <pch.h>
#include "gui/GUIUtility.h"
#include <algorithm>
#include "gui/TextureStore.h"
#include "util/Common.h"
#include "util/StringUtility.h"
#include "fs/FileUtility.h"
#include <SDL3_image/SDL_image.h>
#include <c_resource.h>
#include <tuple>

using namespace fig::gui;
using namespace fig::string_util;
using namespace fig::common_util;

namespace fig::gui_util
{
	bool is_defined(Color color)
	{
		return color.r != 0 || color.g != 0 || color.b != 0 || color.a != 0;
	}

	Color with_alpha(Color color, Uint8 alpha)
	{
		return Color { color.r, color.g, color.b, alpha };
	}

	Color add_rgb(Color colorA, Color colorB)
	{
		float r = std::clamp(toF(colorA.r) + toF(colorB.r), 0.0f, 255.0f);
		float g = std::clamp(toF(colorA.g) + toF(colorB.g), 0.0f, 255.0f);
		float b = std::clamp(toF(colorA.b) + toF(colorB.b), 0.0f, 255.0f);

		return Color {
			static_cast<uint8_t>(r),
			static_cast<uint8_t>(g),
			static_cast<uint8_t>(b),
			colorA.a
		};
	}

	Color add_rgb(Color color, int value)
	{
		int r = std::clamp(toI(color.r) + value, 0, 255);
		int g = std::clamp(toI(color.g) + value, 0, 255);
		int b = std::clamp(toI(color.b) + value, 0, 255);

		return Color {
			static_cast<uint8_t>(r),
			static_cast<uint8_t>(g),
			static_cast<uint8_t>(b),
			color.a
		};
	}

	Color add_rgb(Color color, float value)
	{
		float r = std::clamp(toF(color.r) + value * 255.0f, 0.0f, 255.0f);
		float g = std::clamp(toF(color.g) + value * 255.0f, 0.0f, 255.0f);
		float b = std::clamp(toF(color.b) + value * 255.0f, 0.0f, 255.0f);

		return Color {
			static_cast<uint8_t>(r),
			static_cast<uint8_t>(g),
			static_cast<uint8_t>(b),
			color.a
		};
	}

	Color multiply_rgb(Color colorA, Color colorB)
	{
		return Color {
			static_cast<uint8_t>(toF(colorA.r) * toF(colorB.r) / 255.0f),
			static_cast<uint8_t>(toF(colorA.g) * toF(colorB.g) / 255.0f),
			static_cast<uint8_t>(toF(colorA.b) * toF(colorB.b) / 255.0f),
			colorA.a
		};
	}

	Color multiply_rgb(Color color, float value)
	{
		float r = std::clamp(toF(color.r) * value, 0.0f, 255.0f);
		float g = std::clamp(toF(color.g) * value, 0.0f, 255.0f);
		float b = std::clamp(toF(color.b) * value, 0.0f, 255.0f);

		return Color {
			static_cast<uint8_t>(r),
			static_cast<uint8_t>(g),
			static_cast<uint8_t>(b),
			color.a
		};
	}

	Color color_from_string(fig::string hex)
	{
		hex = trim(hex);
		if (hex.empty())
			return (Color)0;
		if (hex[0] == '#')
			hex = hex.erase(0, 1);
		if (hex.length() != 6 && hex.length() != 8)
			return (Color)0;

		try
		{
			if (hex.length() == 6)
			{
				uint8_t r = static_cast<uint8_t>(std::stoi(hex.substr(0, 2), nullptr, 16));
				uint8_t g = static_cast<uint8_t>(std::stoi(hex.substr(2, 2), nullptr, 16));
				uint8_t b = static_cast<uint8_t>(std::stoi(hex.substr(4, 2), nullptr, 16));
				return Color { r, g, b, 0xff };
			}
			else if (hex.length() == 8)
			{
				uint8_t a = static_cast<uint8_t>(std::stoi(hex.substr(0, 2), nullptr, 16));
				uint8_t r = static_cast<uint8_t>(std::stoi(hex.substr(2, 2), nullptr, 16));
				uint8_t g = static_cast<uint8_t>(std::stoi(hex.substr(4, 2), nullptr, 16));
				uint8_t b = static_cast<uint8_t>(std::stoi(hex.substr(6, 2), nullptr, 16));
				return Color { r, g, b, a };
			}
		}
		catch (...)
		{
			return (Color)0;
		}
		return (Color)0;
	}

	static void RGBtoHSV(uint8_t src_r, uint8_t src_g, uint8_t src_b, float& dst_h, float& dst_s, float& dst_v)
	{
		float r = src_r / 255.0f;
		float g = src_g / 255.0f;
		float b = src_b / 255.0f;

		float h, s, v; // h:0-360.0, s:0.0-1.0, v:0.0-1.0

		float min = std::min(r, std::min(g, b));
		float max = std::max(r, std::max(g, b));

		v = max;

		if (max == 0.0f)
		{
			s = 0;
			h = 0;
		}
		else if (max - min == 0.0f)
		{
			s = 0;
			h = 0;
		}
		else
		{
			s = (max - min) / max;

			if (max == r)
			{
				h = 60 * ((g - b) / (max - min)) + 0;
			}
			else if (max == g)
			{
				h = 60 * ((b - r) / (max - min)) + 120;
			}
			else
			{
				h = 60 * ((r - g) / (max - min)) + 240;
			}
		}

		if (h < 0)
			h += 360.0f;

		dst_h = h;
		dst_s = s;
		dst_v = v;
	}

	static void HSVtoRGB(float h, float s, float v, uint8_t& dst_r, uint8_t& dst_g, uint8_t& dst_b)
	{
		float r, g, b; // 0.0-1.0

		int   hi = (int)(h / 60.0f) % 6;
		float f = (h / 60.0f) - hi;
		float p = v * (1.0f - s);
		float q = v * (1.0f - s * f);
		float t = v * (1.0f - s * (1.0f - f));

		switch (hi)
		{
		case 0: r = v, g = t, b = p; break;
		case 1: r = q, g = v, b = p; break;
		case 2: r = p, g = v, b = t; break;
		case 3: r = p, g = q, b = v; break;
		case 4: r = t, g = p, b = v; break;
		case 5: r = v, g = p, b = q; break;
		}

		dst_r = (uint8_t)(r * 255);
		dst_g = (uint8_t)(g * 255);
		dst_b = (uint8_t)(b * 255);
	}

	void color_to_hsv(Color color, float& h, float& s, float& v)
	{
		RGBtoHSV(color.r, color.g, color.b, h, s, v);
	}

	Color hsv_to_color(float h, float s, float v)
	{
		uint8_t r, g, b;
		HSVtoRGB(h, s, v, r, g, b);
		return Color { r, g, b, 0xff };
	}

	fig::sdl::Surface LoadImageFromMemory(fig::byte_span data)
	{
		SDL_IOStream* io = SDL_IOFromConstMem(data.data(), data.size());
		if (!io)
			return {};

		SDL_Surface* pSurface = IMG_Load_IO(io, true);
		if (!pSurface)
			return {};

		fig::sdl::Surface surface;
		surface.reset(pSurface);
		return surface; // rvo
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

	fig::sdl::Surface ScaleSurface(const fig::sdl::Surface& surface, int32_t width, int32_t height, ImageFit fit)
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
			SDL_StretchSurface(pImage, &srcRect, pSurface, &dstRect, SDL_SCALEMODE_LINEAR);
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

			SDL_StretchSurface(pImage, &srcRect, pSurface, &dstRect, SDL_SCALEMODE_LINEAR);
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
			fig::fs::WriteFile("./mask.bin", data);
			SDL_UnlockSurface(pMask);
		}
	}

	bool MaskCorners(fig::sdl::Surface& surface, CornerStyle style)
	{
		auto pImage = surface.get();
		if (pImage == nullptr || pImage->w <= 0 || pImage->h <= 0)
			return false;

		// Load mask
		fig::path maskPath;
		switch (style)
		{
		case CornerStyle::Card:
			maskPath = fig::path("./resources/masks/card_corners.mask");
			break;
		default:
			return false;
		}

		auto maybe_mask = fig::fs::ReadFile(maskPath); //! @todo cache
		if (not maybe_mask.has_value())
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

		uint8_t* mask_pixels = (uint8_t*)maybe_mask.value().data();
		size_t mask_width	= toUZ(std::sqrt(toI(maybe_mask.value().size()))); // assumes square mask
		size_t mask_pitch	= mask_width;
		size_t corner_size	= mask_width / 2;

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

	fig::sdl::Surface CreateCoverImage(const fig::sdl::Surface& surface)
	{
		auto pSurface = SDL_CreateSurface(Constants::GUI::HomeScreen::CardWidth, Constants::GUI::HomeScreen::CardHeight, SDL_PIXELFORMAT_RGBA8888);
		if (not (bool)pSurface)
			return {};
		
		fig::sdl::Surface cover {};
		cover.reset(pSurface);

		// Draw background
		auto pBGImage = TextureStore::GetImage(TextureType::CARD_DEFAULT_BG);
		SDL_BlitSurface(pBGImage, NULL, pSurface, NULL);

		auto pScaledImage = ScaleSurface(surface, Constants::GUI::HomeScreen::CardWidth, Constants::GUI::HomeScreen::CardHeight, ImageFit::Portrait);
		SDL_BlitSurface(pScaledImage.get(), NULL, pSurface, NULL);

		// Round corners
		MaskCorners(cover, CornerStyle::Card);

		return cover;
	}
}