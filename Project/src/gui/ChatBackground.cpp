#include <pch.h>
#include <execution>
#include "gui/ChatBackground.h"
#include "io/ContentManager.h"

#include <fast_gaussian_blur_template.h>

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
	}
	void ChatBackground::SetAlpha(float alpha)
	{
		_alpha = static_cast<uint8_t>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f);
	}

	void ChatBackground::SetSaturation(float value)
	{
		_saturation = static_cast<uint8_t>(std::clamp(value, 0.0f, 1.0f) * 255.0f);
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

		if (_texture)
		{
			SDL_SetTextureBlendMode(_texture.get(), SDL_BLENDMODE_BLEND);
			SDL_SetTextureColorMod(_texture.get(), _value, _value, _value);
			SDL_SetTextureAlphaMod(_texture.get(), _alpha);

			auto& boundingRect = _bBounded ? _boundingRect : GetRect();
			auto fitRect = ScaleToFit(fig::rect { 0, 0, _texture->w, _texture->h }, boundingRect, _fit);
			auto drawRect = to_rectf(fitRect);
			float fZoom = _fZoom >= 0.0f ? (1.0f + _fZoom) : (1.0f + _fZoom * 0.5f);
			drawRect.w *= fZoom;
			drawRect.h *= fZoom;
			
			drawRect = AlignRect(drawRect, to_rectf(boundingRect), _alignment);
			if (not _bBounded and (_alignment == ImageAlignment::Center or _alignment == ImageAlignment::North or _alignment == ImageAlignment::South))
				drawRect.x = (_boundingRect.x + _boundingRect.w / 2) - (drawRect.w / 2); // Center on chat

			SDL_RenderTexture(pRenderer, _texture.get(), NULL, &drawRect);
		}
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