#include <pch.h>
#include "gui/Image.h"
#include "gui/GUIUtility.h"
#include "gui/AppResources.h"

using namespace fig::util;

namespace fig::gui
{
	Image::Image(LayoutElement* pParent, Texture* pTexture, Color tint) : Control(pParent),
		_pTexture(pTexture)
	{
		if (_pTexture)
			SetSize(_pTexture->w, _pTexture->h);

		SetForegroundColor(tint);
		SetBackgroundColor(Colors::Transparent);
	}

	Image::Image(LayoutElement* pParent, TextureType texture, Color tint) : Control(pParent)
	{
		_pTexture = AppResources::GetTexture(texture);
		if (_pTexture)
			SetSize(_pTexture->w, _pTexture->h);

		SetForegroundColor(tint);
		SetBackgroundColor(Colors::Transparent);
	}

	void Image::OnRender(Renderer* pRenderer)
	{
		auto bgColor = GetBackgroundColor();
		auto fgColor = GetForegroundColor();
		if (bgColor.IsDefined() && bgColor.a != 0)
			DrawBackground(pRenderer);

		if (_pTexture)
		{
			auto rect = GetDrawRect();

			if (fgColor.IsDefined())
				SDL_SetTextureColorMod(_pTexture, fgColor.r, fgColor.g, fgColor.b);
			else
				SDL_SetTextureColorMod(_pTexture, 0xFF, 0xFF, 0xFF);

			if (fgColor.IsDefined() && fgColor.a != 0)
				SDL_SetTextureAlphaMod(_pTexture, fgColor.a);
			else
				SDL_SetTextureAlphaMod(_pTexture, 0xFF);

			if (dbl_eq(_angle, 0.0))
				SDL_RenderTexture(pRenderer, _pTexture, NULL, &rect);
			else
				SDL_RenderTextureRotated(pRenderer, _pTexture, NULL, &rect, _angle, NULL, SDL_FLIP_NONE);
		}
	}

	void Image::SetTexture(TexturePtr pTexture, bool bResize)
	{
		_pTexture = pTexture;
		if (bResize and pTexture)
			SetSize(pTexture->w, pTexture->h);
	}

	void Image::SetTexture(TextureType texture, bool bResize)
	{
		SetTexture(AppResources::GetTexture(texture), bResize);
	}

	Point Image::GetTextureSize() const noexcept
	{
		if (_pTexture)
			return Point { _pTexture->w, _pTexture->h };
		return {};
	}
}