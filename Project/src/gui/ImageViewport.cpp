#include <pch.h>
#include "gui/ImageViewport.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"
#include "user/UserSettings.h"

using namespace fig::io;

namespace fig::gui
{
	ImageViewport::ImageViewport(ControlPtr pParent, bool bClampZoom) : Control(pParent),
		_bClampZoom { bClampZoom }
	{
		SetForegroundColor(Color::White);
		SetBackgroundColor(Color::Transparent);
 
		auto pBorder = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_6PX, 8);
		pBorder->SetColor(Color::Border);
		SetMask(AppResources::GetTexture(Resource::MASK_CARD), 8);
	}

	void ImageViewport::OnRender(fig::renderer_ptr pRenderer)
	{
		DrawBackground(pRenderer);

		auto bgColor = GetBackgroundColor();
		auto fgColor = GetForegroundColor();
		if (bgColor && bgColor.a() != 0)
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
				SDL_SetTextureColorMod(pTexture, fgColor.r(), fgColor.g(), fgColor.b());
			else
				SDL_SetTextureColorMod(pTexture, 0xFF, 0xFF, 0xFF);

			if (fgColor.IsDefined() && fgColor.a() != 0)
				SDL_SetTextureAlphaMod(pTexture, fgColor.a());
			else
				SDL_SetTextureAlphaMod(pTexture, 0xFF);

			SDL_RenderTexture(pRenderer, pTexture, NULL, &rect);
		}
	}

	EventResult ImageViewport::OnEvent(fig::event& event)
	{
		switch (event.type)
		{
		case SDL_EVENT_MOUSE_MOTION:
			HandleMouseMotion(event.motion);
			return EventResult::Continue;
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			return HandleMouseDown(event.button) ? EventResult::Handled : EventResult::Pass;
		case SDL_EVENT_MOUSE_BUTTON_UP:
			return HandleMouseUp(event.button) ? EventResult::Handled : EventResult::Pass;
		case SDL_EVENT_MOUSE_WHEEL:
			return HandleMouseWheel(event.wheel) ? EventResult::Handled : EventResult::Pass;
		}

		return EventResult::Pass;
	}

	float ImageViewport::GetFitScale(fig::coord width, fig::coord height) const
	{
		float dstWidth = toF(width);
		float dstHeight = toF(height);
		float srcWidth = toF(_imageSize.x);
		float srcHeight = toF(_imageSize.y);

		if ((srcWidth > dstWidth and srcHeight > dstHeight) or not _bClampZoom)
			return std::max(dstWidth / srcWidth, dstHeight / srcHeight);
		return 1.0f;
	}

	void ImageViewport::ClampOffset(fig::point& offset, fig::coord width, fig::coord height)
	{
		fig::rectf drawRect = CalcDrawRect(width, height, _fZoom, offset);
		float maxOffsetX = std::max(0.0f, (drawRect.w - toF(width)) / 2.0f);
		float maxOffsetY = std::max(0.0f, (drawRect.h - toF(height)) / 2.0f);
		offset.x = std::clamp(offset.x, toI(-maxOffsetX), toI(maxOffsetX));
		offset.y = std::clamp(offset.y, toI(-maxOffsetY), toI(maxOffsetY));
	}

	fig::rectf ImageViewport::CalcDrawRect(fig::coord width, fig::coord height, float zoom, fig::point offset) const
	{
		float dstWidth = toF(width);
		float dstHeight = toF(height);
		float srcWidth = toF(_imageSize.x);
		float srcHeight = toF(_imageSize.y);
		float scale = GetFitScale(width, height) * zoom;

		if (_bClampZoom)
			scale = std::min(scale, 1.0f);

		fig::rectf drawRect;
		drawRect.w = srcWidth * scale;
		drawRect.h = srcHeight * scale;
		drawRect.x = (dstWidth - drawRect.w) / 2.0f - toF(offset.x);
		drawRect.y = (dstHeight - drawRect.h) / 2.0f - toF(offset.y);

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

	bool ImageViewport::HandleMouseWheel(SDL_MouseWheelEvent& event)
	{
		auto& rect = GetRect();
		fig::point pt = { toI(event.mouse_x), toI(event.mouse_y) };
		if (!SDL_PointInRect(&pt, &rect))
			return false;

		if (_fZoom == 1.0f && event.integer_y < 0)
			return true; // Ignore, clamped

		auto width = GetWidth();
		auto height = GetHeight();

		fig::rectf oldDrawRect = CalcDrawRect(width, height, _fZoom, _offset);
		fig::point mouseLocal = { pt.x - rect.x, pt.y - rect.y };
		float fractionX = (toF(mouseLocal.x) - oldDrawRect.x) / oldDrawRect.w;
		float fractionY = (toF(mouseLocal.y) - oldDrawRect.y) / oldDrawRect.h;

		float newZoom = _fZoom * std::pow(Constants::GUI::Chat::ImageZoomFactor, toF(event.integer_y));
		_fZoom = std::max(newZoom, 1.0f);
		_fZoom = std::min(_fZoom, 1.0f / GetFitScale(width, height));
		
		fig::rectf newDrawRect = CalcDrawRect(width, height, newZoom, { 0, 0 });
		float newDrawRectX = toF(mouseLocal.x) - fractionX * newDrawRect.w;
		float newDrawRectY = toF(mouseLocal.y) - fractionY * newDrawRect.h;

		_offset.x = toI(newDrawRect.x - newDrawRectX);
		_offset.y = toI(newDrawRect.y - newDrawRectY);
		ClampOffset(_offset, width, height);

		SetDirty();
		return true;
	}

	bool ImageViewport::HandleMouseDown(SDL_MouseButtonEvent& event)
	{
		auto rect = GetRect();
		if (_bMouseDown or event.button != SDL_BUTTON_LEFT or not is_inside(rect, toI(event.x), toI(event.y)))
			return false;

		_bMouseDown = true;
		_mouseDownPos = fig::point { toI(event.x), toI(event.y) };
		_mouseDownOffset = _offset;

		return true;
	}

	bool ImageViewport::HandleMouseUp(SDL_MouseButtonEvent& event)
	{
		if (not (_bMouseDown and event.button == SDL_BUTTON_LEFT))
			return false;

		_bMouseDown = false;
		return true;
	}

	bool ImageViewport::HandleMouseMotion(SDL_MouseMotionEvent& motionEvent)
	{
		if (not _bMouseDown)
			return false;

		_offset.x = _mouseDownOffset.x - (toI(motionEvent.x) - _mouseDownPos.x);
		_offset.y = _mouseDownOffset.y - (toI(motionEvent.y) - _mouseDownPos.y);
		ClampOffset(_offset, GetWidth(), GetHeight());

		SetDirty();

		return true;
	}

	void ImageViewport::SetTexture(fig::texture_ptr pTexture) noexcept
	{
		_pTexture = pTexture;
		
		if (_pTexture)
		{
			_imageSize = fig::point { _pTexture->w, _pTexture->h };
			_fImageRatio = toF(_imageSize.x) / toF(_imageSize.y);
		}
		else
		{
			_imageSize = fig::point {};
			_fImageRatio = 1.0f;
		}

		ResetTransform();
		SetDirty();
	}

	void ImageViewport::SetBackgroundTexture(fig::texture_ptr pBGTexture) noexcept
	{
		_pBGTexture = pBGTexture;
		SetDirty();
	}

	void ImageViewport::SetMask(fig::texture_ptr pTexture, fig::coord cornerSize) noexcept
	{
		_pMask = pTexture;
		_cornerSize = cornerSize;
		_bRedraw = true;
		_bRedrawAlpha = true;
	}

	void ImageViewport::Redraw()
	{
		auto width = std::min(GetWidth(), 2048);
		auto height = std::min(GetHeight(), 2048);

		if (width != _lastSize.x or height != _lastSize.y)
		{
			_lastSize = fig::point { width, height };
			_targetTexture.clear();
			ClampOffset(_offset, GetWidth(), GetHeight());
		}

		auto pRenderer = GetSDLRenderer();
		SDL_assert(pRenderer);

		fig::texture_ptr pTarget = _targetTexture.get();
		if (!pTarget)
		{
			pTarget = SDL_CreateTexture(pRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
			_targetTexture.reset(pTarget);
			_bRedrawAlpha = true;
		}

		auto priorRenderTarget = SDL_GetRenderTarget(pRenderer);
		SDL_SetRenderTarget(pRenderer, pTarget);

		fig::rectf drawRect = CalcDrawRect(GetWidth(), GetHeight(), _fZoom, _offset);

		// Render with alpha
		if (_pMask)
		{
			if (_bRedrawAlpha)
			{
				SDL_SetRenderDrawColor(pRenderer, 0, 0, 0, 0);
				SDL_RenderClear(pRenderer);
				SDL_SetTextureBlendMode(_pMask, SDL_BLENDMODE_NONE);
				if (_cornerSize)
					SDL_RenderTexture9Grid(pRenderer, _pMask, NULL, toF(_cornerSize), toF(_cornerSize), toF(_cornerSize), toF(_cornerSize), 1.0f, NULL);
				else
					SDL_RenderTexture(pRenderer, _pMask, NULL, NULL);
			}

			SDL_BlendMode blendMode = SDL_ComposeCustomBlendMode(
				SDL_BLENDFACTOR_SRC_ALPHA, 
				SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, 
				SDL_BLENDOPERATION_ADD,
				SDL_BLENDFACTOR_ZERO, 
				SDL_BLENDFACTOR_ONE, 
				SDL_BLENDOPERATION_ADD);


			// Render background
			if (_pBGTexture)
			{
				SDL_SetTextureBlendMode(_pBGTexture, blendMode);
				SDL_SetTextureColorMod(_pBGTexture, 0xFF, 0xFF, 0xFF);
				SDL_SetTextureAlphaMod(_pBGTexture, 0xFF);
				SDL_RenderTexture(pRenderer, _pBGTexture, NULL, NULL);
			}
			else
			{
				auto bgColor = GetBackgroundColor();
				SDL_SetRenderDrawBlendMode(pRenderer, blendMode);
				SDL_SetRenderDrawColor(pRenderer, bgColor.r(), bgColor.g(), bgColor.b(), 255);
				SDL_RenderFillRect(pRenderer, NULL);
				SDL_SetRenderDrawBlendMode(pRenderer, SDL_BLENDMODE_BLEND);
			}

			// Render texture
			if (_pTexture)
			{
				SDL_SetTextureBlendMode(_pTexture, blendMode);
				SDL_SetTextureColorMod(_pTexture, 0xFF, 0xFF, 0xFF);
				SDL_SetTextureAlphaMod(_pTexture, 0xFF);
				SDL_RenderTexture(pRenderer, _pTexture, NULL, &drawRect);
			}
		}
		else
		{
			// Render background
			if (_pBGTexture)
			{
				SDL_SetTextureBlendMode(_pBGTexture, SDL_BLENDMODE_BLEND);
				SDL_SetTextureColorMod(_pBGTexture, 0xFF, 0xFF, 0xFF);
				SDL_SetTextureAlphaMod(_pBGTexture, 0xFF);
				SDL_RenderTexture(pRenderer, _pBGTexture, NULL, NULL);
			}
			else
			{
				auto bgColor = GetBackgroundColor();
				SDL_SetRenderDrawBlendMode(pRenderer, SDL_BLENDMODE_BLEND);
				SDL_SetRenderDrawColor(pRenderer, bgColor.r(), bgColor.g(), bgColor.b(), 255);
				SDL_RenderFillRect(pRenderer, NULL);
			}

			if (_pTexture)
			{
				SDL_SetTextureBlendMode(_pTexture, SDL_BLENDMODE_BLEND);
				SDL_SetTextureColorMod(_pTexture, 0xFF, 0xFF, 0xFF);
				SDL_SetTextureAlphaMod(_pTexture, 0xFF);
				SDL_RenderTexture(pRenderer, _pTexture, NULL, &drawRect);
			}
		}

		SDL_SetRenderTarget(pRenderer, priorRenderTarget);
	}

	void ImageViewport::ResetTransform()
	{
		_fZoom = 1.0f;
		_offset = {0, -_imageSize.y };
		ClampOffset(_offset, GetWidth(), GetHeight());
		SetDirty();
	}

	void ImageViewport::SetDirty()
	{
		_bRedraw = true;
	}

	void ImageViewport::OnSize()
	{
		SetDirty();
	}

}