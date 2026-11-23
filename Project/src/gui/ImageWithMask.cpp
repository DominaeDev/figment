#include <pch.h>
#include "gui/ImageWithMask.h"
#include "gui/GUIUtility.h"

namespace fig::gui
{
	ImageWithMask::ImageWithMask(ControlPtr pParent, const fig::sdl::Surface& surface, fig::texture_ptr pMask) : Control(pParent)
	{
		SetTexture(surface, pMask, true);
		SetForegroundColor(Color::White);
		SetBackgroundColor(Color::Transparent);
	}

	void ImageWithMask::OnRender(fig::renderer_ptr pRenderer)
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

		if (auto pTexture = _targetTexture.get(); pTexture and fgColor.a() > 0)
		{
			auto rect = GetDrawRect();

			if (fgColor.IsDefined())
				SDL_SetTextureColorMod(pTexture, fgColor.r(), fgColor.g(), fgColor.b());
			else
				SDL_SetTextureColorMod(pTexture, 0xFF, 0xFF, 0xFF);

			SDL_SetTextureAlphaMod(pTexture, fgColor.a());
			SDL_RenderTexture(pRenderer, pTexture, NULL, &rect);
		}
	}

	void ImageWithMask::SetTexture(const fig::sdl::Surface& surface, fig::texture_ptr pMask, bool bResize)
	{
		Clear();
		if (auto pTexture = SDL_CreateTextureFromSurface(GetSDLRenderer(), surface.get()))
		{
			_texture.reset(pTexture);
			_pMask = pMask;
			if (bResize and pTexture)
				SetSize(pTexture->w, pTexture->h);
			SetDirty();
		}
	}

	fig::point ImageWithMask::GetTextureSize() const noexcept
	{
		if (_texture)
			return fig::point { _texture.get()->w, _texture.get()->h };
		return {};
	}
	
	void ImageWithMask::SetMask(fig::texture_ptr pMask, fig::coord cornerSize)
	{
		_pMask = pMask;
		_cornerSize = cornerSize;
		SetDirty();
	}

	void ImageWithMask::Clear()
	{
		_texture.clear();
		_targetTexture.clear();
		_pMask = nullptr;
		SetDirty();
	}

	void ImageWithMask::Redraw()
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
			if (_cornerSize)
				SDL_RenderTexture9Grid(pRenderer, _pMask, NULL, toF(_cornerSize), toF(_cornerSize), toF(_cornerSize), toF(_cornerSize), 1.0f, NULL);
			else
				SDL_RenderTexture(pRenderer, _pMask, NULL, NULL);

			SDL_BlendMode blendMode = SDL_ComposeCustomBlendMode(
				SDL_BLENDFACTOR_SRC_ALPHA,
				SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
				SDL_BLENDOPERATION_ADD,
				SDL_BLENDFACTOR_ZERO,
				SDL_BLENDFACTOR_ONE,
				SDL_BLENDOPERATION_ADD);

			// Render texture
			if (auto pTexture = _texture.get())
			{
				SDL_SetTextureBlendMode(pTexture, blendMode);
				SDL_SetTextureColorMod(pTexture, 0xFF, 0xFF, 0xFF);
				SDL_SetTextureAlphaMod(pTexture, 0xFF);
				SDL_RenderTexture(pRenderer, pTexture, NULL, NULL);
			}
		}
		else
		{
			if (auto pTexture = _texture.get())
			{
				SDL_SetTextureBlendMode(pTexture, SDL_BLENDMODE_BLEND);
				SDL_SetTextureColorMod(pTexture, 0xFF, 0xFF, 0xFF);
				SDL_SetTextureAlphaMod(pTexture, 0xFF);
				SDL_RenderTexture(pRenderer, pTexture, NULL, NULL);
			}
		}

		SDL_SetRenderTarget(pRenderer, priorRenderTarget);
	}

	void ImageWithMask::SetDirty()
	{
		_bDirty = true;
	}

	void ImageWithMask::OnSize()
	{
		SetDirty();
	}
}