#include <pch.h>
#include "gui/ImageWithMask.h"
#include "gui/GUIUtility.h"

namespace fig::gui
{
	ImageWithMask::ImageWithMask(ParentPtr pParent, fig::texture_ptr pTexture, fig::texture_ptr pMask, fig::color tint) : Control(pParent)
	{
		SetTexture(pTexture, pMask, true);
		SetForegroundColor(tint);
		SetBackgroundColor(Colors::Transparent);
	}

	void ImageWithMask::OnRender(fig::renderer_ptr pRenderer)
	{
		auto bgColor = GetBackgroundColor();
		auto fgColor = GetForegroundColor();
		if (bgColor.IsDefined() && bgColor.a != 0)
			DrawBackground(pRenderer);

		if (auto pTexture = _texture.get())
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

	void ImageWithMask::SetTexture(fig::texture_ptr pTexture, fig::texture_ptr pMask, bool bResize)
	{
		if (!pTexture)
		{
			_texture.clear();
			return;
		}
		if (bResize)
			SetSize(pTexture->w, pTexture->h);

		auto pRenderer = GetSDLRenderer();
		SDL_assert(pRenderer);

		// Bake mask into texture
		if (pMask)
		{
			fig::texture_ptr pTarget = SDL_CreateTexture(pRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, pTexture->w, pTexture->h);
			auto priorRenderTarget = SDL_GetRenderTarget(pRenderer);

			SDL_SetRenderTarget(pRenderer, pTarget);
			SDL_SetRenderDrawColor(pRenderer, 0, 0, 0, 0);
			SDL_RenderClear(pRenderer);
			SDL_SetTextureBlendMode(pMask, SDL_BLENDMODE_NONE);
			SDL_RenderTexture(pRenderer, pMask, NULL, NULL);

			SDL_BlendMode multiplyAlpha = SDL_ComposeCustomBlendMode(
				SDL_BLENDFACTOR_DST_ALPHA,
				SDL_BLENDFACTOR_ZERO,
				SDL_BLENDOPERATION_ADD,
				SDL_BLENDFACTOR_ZERO,
				SDL_BLENDFACTOR_ONE,
				SDL_BLENDOPERATION_ADD
			);

			SDL_SetTextureBlendMode(pTexture, multiplyAlpha);
			SDL_RenderTexture(pRenderer, pTexture, NULL, NULL);
			SDL_SetRenderTarget(pRenderer, priorRenderTarget);
			SDL_SetTextureBlendMode(pTarget, SDL_BLENDMODE_BLEND_PREMULTIPLIED);
			_texture.reset(pTarget);
		}
		else
		{
			_texture.reset(pTexture);
		}
	}

	fig::point ImageWithMask::GetTextureSize() const noexcept
	{
		if (_texture)
			return fig::point { _texture.get()->w, _texture.get()->h };
		return {};
	}

	void ImageWithMask::Reset()
	{
		_texture.clear();
	}
}