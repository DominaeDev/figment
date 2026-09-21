#include <pch.h>
#include "gui/NonOwningImageWithMask.h"
#include "gui/GUIUtility.h"

namespace fig::gui
{
	NonOwningImageWithMask::NonOwningImageWithMask(ControlPtr pParent, fig::texture_ptr pTexture, fig::texture_ptr pMask) : Control(pParent)
	{
		SetTexture(pTexture, pMask);
		SetForegroundColor(Colour::White);
		SetBackgroundColor(Colour::Transparent);
	}

	void NonOwningImageWithMask::OnRender(fig::renderer_ptr pRenderer)
	{
		if (_bDirty)
		{
			Redraw();
			_bDirty = false;
		}

		auto bgColor = GetBackgroundColor();
		auto fgColor = GetForegroundColor();
		if (bgColor && bgColor.a() != 0)
			DrawBackground(pRenderer);

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

	void NonOwningImageWithMask::SetTexture(fig::texture_ptr pTexture, fig::texture_ptr pMask, bool bResize)
	{
		_pTexture = pTexture;
		_pMask = pMask;
		if (bResize)
			SetSize(pTexture->w, pTexture->h);
		SetDirty();
	}

	void NonOwningImageWithMask::Redraw()
	{
		auto width = std::min(GetWidth(), 2048);
		auto height = std::min(GetHeight(), 2048);

		if (width != _lastSize.x or height != _lastSize.y)
		{
			_lastSize = fig::point { width, height };
			_targetTexture.clear();
		}

		auto pRenderer = GetSDLRenderer();
		SDL_assert(pRenderer);

		fig::texture_ptr pTarget = _targetTexture.get();
		if (!pTarget)
		{
			pTarget = SDL_CreateTexture(pRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
			_targetTexture.reset(pTarget);
		}

		auto priorRenderTarget = SDL_GetRenderTarget(pRenderer);
		SDL_SetRenderTarget(pRenderer, pTarget);

		// Render with alpha
		if (_pMask)
		{
			SDL_SetRenderDrawColor(pRenderer, 0, 0, 0, 0);
			SDL_RenderClear(pRenderer);
			SDL_SetTextureBlendMode(_pMask, SDL_BLENDMODE_NONE);
			SDL_RenderTexture(pRenderer, _pMask, NULL, NULL);

			SDL_BlendMode blendMode = SDL_ComposeCustomBlendMode(
				SDL_BLENDFACTOR_SRC_ALPHA,
				SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
				SDL_BLENDOPERATION_ADD,
				SDL_BLENDFACTOR_ZERO,
				SDL_BLENDFACTOR_ONE,
				SDL_BLENDOPERATION_ADD);

			// Render texture
			if (_pTexture)
			{
				SDL_SetTextureBlendMode(_pTexture, blendMode);
				SDL_SetTextureColorMod(_pTexture, 0xFF, 0xFF, 0xFF);
				SDL_SetTextureAlphaMod(_pTexture, 0xFF);
				SDL_RenderTexture(pRenderer, _pTexture, NULL, NULL);
			}
		}
		else
		{
			if (_pTexture)
			{
				SDL_SetTextureBlendMode(_pTexture, SDL_BLENDMODE_BLEND);
				SDL_SetTextureColorMod(_pTexture, 0xFF, 0xFF, 0xFF);
				SDL_SetTextureAlphaMod(_pTexture, 0xFF);
				SDL_RenderTexture(pRenderer, _pTexture, NULL, NULL);
			}
		}

		SDL_SetRenderTarget(pRenderer, priorRenderTarget);
	}

	void NonOwningImageWithMask::SetDirty()
	{
		_bDirty = true;
	}

	void NonOwningImageWithMask::OnSize()
	{
		SetDirty();
	}
}