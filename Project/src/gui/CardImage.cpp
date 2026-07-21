#include <pch.h>
#include "gui/CardImage.h"

namespace fig::gui
{
	CardImage::CardImage(ParentPtr pParent, TexturePtr pTexture, TexturePtr pMask) noexcept : Control(pParent),
		_pTexture(pTexture),
		_pMask(pMask)
	{
		if (pTexture)
			SetSize(pTexture->w, pTexture->h);

		SetForegroundColor(Colors::White);
		SetBackgroundColor(Colors::Transparent);
	}

	void CardImage::OnRender(Renderer* pRenderer)
	{
		auto bgColor = GetBackgroundColor();
		auto fgColor = GetForegroundColor();
		if (bgColor.IsDefined() && bgColor.a != 0)
			DrawBackground(pRenderer);

		if (_bRedraw)
		{
			Redraw();
			_bRedraw = false;
			_bRedrawAlpha = false;
		}

		if (auto pTexture = _targetTexture.get())
		{
			auto rect = GetDrawRect();

			if (fgColor.IsDefined())
				SDL_SetTextureColorMod(pTexture, fgColor.r, fgColor.g, fgColor.b);
			else
				SDL_SetTextureColorMod(pTexture, 0xFF, 0xFF, 0xFF);

			if (fgColor.IsDefined() && fgColor.a != 0)
				SDL_SetTextureAlphaMod(pTexture, fgColor.a);
			else
				SDL_SetTextureAlphaMod(pTexture, 0xFF);

			SDL_RenderTexture(pRenderer, pTexture, NULL, &rect);
		}
	}

	void CardImage::SetTexture(TexturePtr pTexture, bool bResize) noexcept
	{
		_pTexture = pTexture;
		if (bResize and pTexture)
			SetSize(pTexture->w, pTexture->h);
		SetDirty();
	}

	void CardImage::SetMask(TexturePtr pTexture) noexcept
	{
		_pMask = pTexture;
		_bRedraw = true;
		_bRedrawAlpha = true;
	}

	void CardImage::Redraw()
	{
		auto width = std::min(GetWidth(), 2048);
		auto height = std::min(GetHeight(), 2048);

		if (not (_pTexture and width > 0 and height > 0))
		{
			_targetTexture.clear();
			return; // Error
		}

		if (width != _lastSize.x or height != _lastSize.y)
		{
			_lastSize = Point { width, height };
			_targetTexture.clear();
		}

		auto pRenderer = GetSDLRenderer();
		SDL_assert(pRenderer);

		TexturePtr pTarget = _targetTexture.get();
		if (!pTarget)
		{
			pTarget = SDL_CreateTexture(pRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
			SDL_SetTextureBlendMode(pTarget, SDL_BLENDMODE_BLEND_PREMULTIPLIED);

			_targetTexture.reset(pTarget);
			_bRedrawAlpha = true;
		}

		SDL_SetRenderTarget(pRenderer, pTarget);
		
		constexpr float fCorner = 8.0f;

		// Render with alpha
		if (_pMask)
		{
			if (_bRedrawAlpha)
			{
				SDL_SetRenderDrawColor(pRenderer, 0, 0, 0, 0);
				SDL_RenderClear(pRenderer);
				SDL_SetTextureBlendMode(_pMask, SDL_BLENDMODE_NONE);
				SDL_RenderTexture9Grid(pRenderer, _pMask, nullptr, fCorner, fCorner, fCorner, fCorner, 1.0f, NULL);

				SDL_BlendMode multiplyAlpha = SDL_ComposeCustomBlendMode(
					SDL_BLENDFACTOR_DST_ALPHA,
					SDL_BLENDFACTOR_ZERO,
					SDL_BLENDOPERATION_ADD,
					SDL_BLENDFACTOR_ZERO,
					SDL_BLENDFACTOR_ONE,
					SDL_BLENDOPERATION_ADD
				);
				SDL_SetTextureBlendMode(_pTexture, multiplyAlpha);
			}

			float fWidth = toF(width);
			float fHeight = toF(height);

			Rectf drawRect { 0, 0, toF(_pTexture->w), toF(_pTexture->h) };

			// Center
			drawRect.x -= toF(_pTexture->w - width) / 2.0f;
			drawRect.y -= toF(_pTexture->h - height) / 2.0f;

			float fZoom = _fZoom - 1.0f;
			float fRatio = toF(_pTexture->h) / toF(_pTexture->w);

			// Expand rect uniformally
			float fExpandX = fZoom * _fZoomExpand;
			float fExpandY = fZoom * _fZoomExpand * fRatio;
			drawRect.x -= fExpandX;
			drawRect.y -= fExpandY;
			drawRect.w += fExpandX * 2.0f;
			drawRect.h += fExpandY * 2.0f;

			// Shift upwards (towards the face)
			drawRect.y += _fZoom * _fZoomExpand * fRatio * Constants::GUI::Cards::ZoomVerticalShift;

			if (flt_eq(_fZoom, 1.0f))
			{
				drawRect.x = std::roundf(drawRect.x);
				drawRect.y = std::roundf(drawRect.y);
				drawRect.w = std::roundf(drawRect.w);
				drawRect.h = std::roundf(drawRect.h);
			}

			SDL_RenderTexture(pRenderer, _pTexture, NULL, &drawRect);
		}
		else
		{
			SDL_RenderTexture(pRenderer, _pTexture, NULL, NULL);
		}

		SDL_SetRenderTarget(pRenderer, NULL);
	}

	void CardImage::SetZoom(float value) noexcept
	{
		if (not flt_eq(value, _fZoom))
		{
			_fZoom = value;
			SetDirty();
		}
	}

	void CardImage::SetDirty()
	{
		_bRedraw = true;
	}

	void CardImage::OnSize()
	{
		SetDirty();
	}
}