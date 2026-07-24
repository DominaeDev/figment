#include <pch.h>
#include <execution>
#include "gui/ChatBackground.h"

#include "fast_gaussian_blur_template.h"

namespace fig::gui
{
	ChatBackground::ChatBackground(ControlPtr parent) : Control(parent)
	{
		EnableClipping(true);
		SetBackgroundColor(Color::Transparent);
	}

	void ChatBackground::SetBrightness(float value)
	{
		_value = static_cast<uint8_t>(std::clamp(value, 0.0f, 1.0f) * 255.0f);
		_bDirty = true;
	}

	void ChatBackground::SetBrightness(uint8_t value)
	{
		_value = value;
		_bDirty = true;
	}

	void ChatBackground::SetSaturation(float value)
	{
		_saturation = static_cast<uint8_t>(std::clamp(value, 0.0f, 1.0f) * 255.0f);
		_bDirty = true;
	}

	void ChatBackground::SetAlpha(float alpha)
	{
		_alpha = static_cast<uint8_t>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f);
		_bDirty = true;
	}

	void ChatBackground::SetAlpha(uint8_t alpha)
	{
		_alpha = alpha;
		_bDirty = true;
	}

	void ChatBackground::SetBlur(float sigma)
	{
		_fBlurSigma = std::max(sigma, 0.0f);
		_bDirty = true;
	}

	void ChatBackground::SetImage(const fig::uuid& assetId)
	{
		if (auto try_surface = Global::GetUserContent().Get<fig::sdl::Surface>(assetId))
		{
			auto pNewSurface = SDL_CreateSurface((*try_surface)->w, (*try_surface)->h, SDL_PIXELFORMAT_RGB24);
			SDL_BlitSurface((*try_surface).get(), NULL, pNewSurface, NULL);
			_surface.reset(pNewSurface);
			_processedSurface.clear();

			_imageSize = fig::point { pNewSurface->w, pNewSurface->h };
			_fImageRatio = toF(_imageSize.x) / toF(_imageSize.y);
		}

		_texture.clear();
		_bDirty = true;
	}

	void ChatBackground::OnUpdate(float fElapsed)
	{
		if (_bDirty)
			ProcessImage();
	}

	void ChatBackground::OnRender(fig::renderer_ptr pRenderer)
	{
		if (_surface.empty())
			return;

		auto& surface = not _processedSurface.empty() ? _processedSurface : _surface;
		if (_texture.empty())
		{
			auto pTexture = SDL_CreateTextureFromSurface(pRenderer, surface.get());
			_texture.reset(pTexture);
		}

		SDL_SetTextureBlendMode(_texture.get(), SDL_BLENDMODE_BLEND);
		SDL_SetTextureColorMod(_texture.get(), _value, _value, _value);
		SDL_SetTextureAlphaMod(_texture.get(), _alpha);

		auto drawRect = GetImageRect();
		SDL_RenderTexture(pRenderer, _texture.get(), NULL, &drawRect);
	}

	fig::rectf ChatBackground::GetImageRect() const
	{
		float dstWidth = toF(GetWidth());
		float dstHeight = toF(GetHeight());
		float srcWidth = toF(_imageSize.x);
		float srcHeight = toF(_imageSize.y);
		float scale;

		if (_fit == ImageFit::Inside)
			scale = std::min(toF(dstWidth) / srcWidth, toF(dstHeight) / srcHeight);
		else if (_fit == ImageFit::Outside || _fit == ImageFit::Portrait)
			scale = std::max(toF(dstWidth) / srcWidth, toF(dstHeight) / srcHeight);
		else
			scale = 1.0f;

		fig::rectf drawRect;
		drawRect.w = srcWidth * scale;
		drawRect.h = srcHeight * scale;
		drawRect.x = (dstWidth - drawRect.w) / 2.0f;
		if (_fit == ImageFit::Portrait)
			drawRect.y = 0.0f;
		else
			drawRect.y = (dstHeight - drawRect.h) / 2.0f;
		drawRect.x += GetRect().x;
		drawRect.y += GetRect().y;

		if (flt_eq(scale, 1.0f))
		{
			// Round to nearest pixel
			drawRect.x = std::roundf(drawRect.x);
			drawRect.y = std::roundf(drawRect.y);
			drawRect.w = std::roundf(drawRect.w);
			drawRect.h = std::roundf(drawRect.h);
		}

		return drawRect;
	}

	void ChatBackground::ProcessImage()
	{
		_processedSurface.reset();
		_texture.clear();
		_bDirty = false;

		if (_surface.empty())
			return;
		if (_saturation == 0xFF and _fBlurSigma <= 0.0f)
			return;

		auto pNewSurface = SDL_CreateSurface(_surface->w, _surface->h, SDL_PIXELFORMAT_RGBA8888); // fast_gaussian_blur doesn't appear to work with RGB24
		SDL_BlitSurface(_surface.get(), NULL, pNewSurface, NULL); // Copy original surface
		_processedSurface.reset(pNewSurface);
		Saturate();
		Blur();
	}

	void ChatBackground::Blur()
	{
		if (_processedSurface.empty() or _fBlurSigma <= 0.0f)
			return;

		auto pSurface = _processedSurface.get();

		DEBUG_MEASURE_BEGIN("Blur");

		// Rescale image
		constexpr int32_t MaxSize = 768;
		fig::point size { _surface->w, _surface->h };
		if (size.x > MaxSize or size.y > MaxSize)
		{
			float scale = std::min(toF(MaxSize) / size.x, toF(MaxSize) / size.y);
			size.x = static_cast<int32_t>(toF(size.x) * scale);
			size.y = static_cast<int32_t>(toF(size.y) * scale);

			pSurface = SDL_ScaleSurface(pSurface, size.x, size.y, SDL_SCALEMODE_NEAREST);
			_processedSurface.reset(pSurface);
		}
		
		// Blur
		if (SDL_LockSurface(pSurface))
		{
			auto components = pSurface->pitch / pSurface->w;
			auto pixels = static_cast<unsigned char*>(pSurface->pixels);
			size_t length = pSurface->w * pSurface->h * components;
			
			std::vector<unsigned char> new_pixels(length);
			unsigned char* new_pixel_data = new_pixels.data();
			fast_gaussian_blur(pixels, new_pixel_data, pSurface->w, pSurface->h, components, _fBlurSigma, 2U, kExtend);
			std::memcpy(pixels, new_pixels.data(), length);
			SDL_UnlockSurface(pSurface);
		}
		DEBUG_MEASURE_END();
	}

	void ChatBackground::Saturate()
	{
		if (_processedSurface.empty() or _saturation == 0xFF)
			return;

		auto pSurface = _processedSurface.get();
		auto pFormatDetails = SDL_GetPixelFormatDetails(pSurface->format);
		auto pitchInPixels = pSurface->pitch / 4;

		DEBUG_MEASURE_BEGIN("Desaturate");

		if (SDL_LockSurface(pSurface))
		{
			auto pPixels = static_cast<uint32_t*>(pSurface->pixels);

			auto rowIndices = std::views::iota(0, pSurface->h);
			std::for_each(std::execution::par_unseq, 
				rowIndices.begin(), rowIndices.end(), 
				[&](int row) 
				{
					auto pRow = pPixels + row * pitchInPixels;
					for (int col = 0; col < pSurface->w; col++)
					{
						uint32_t pixel = pRow[col];
						uint8_t r = static_cast<uint8_t>((pixel & pFormatDetails->Rmask) >> pFormatDetails->Rshift);
						uint8_t g = static_cast<uint8_t>((pixel & pFormatDetails->Gmask) >> pFormatDetails->Gshift);
						uint8_t b = static_cast<uint8_t>((pixel & pFormatDetails->Bmask) >> pFormatDetails->Bshift);

						uint8_t luma = static_cast<uint8_t>((77 * r + 151 * g + 28 * b) >> 8);
						uint8_t newR = static_cast<uint8_t>(luma + (((r - luma) * _saturation) >> 8));
						uint8_t newG = static_cast<uint8_t>(luma + (((g - luma) * _saturation) >> 8));
						uint8_t newB = static_cast<uint8_t>(luma + (((b - luma) * _saturation) >> 8));

						pRow[col] = (pixel & ~(pFormatDetails->Rmask | pFormatDetails->Gmask | pFormatDetails->Bmask))
							| (static_cast<uint32_t>(newR) << pFormatDetails->Rshift)
							| (static_cast<uint32_t>(newG) << pFormatDetails->Gshift)
							| (static_cast<uint32_t>(newB) << pFormatDetails->Bshift);
					}
				}
			);
			SDL_UnlockSurface(pSurface);
		}
		DEBUG_MEASURE_END();
	}
}