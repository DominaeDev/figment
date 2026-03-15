#include <pch.h>
#include "gui/NineGridImage.h"

namespace fig::gui
{
	NineGridImage::NineGridImage(LayoutElement* pParent, Texture* pTexture, float cornerPixels) : Control(pParent),
		_pTexture { pTexture }
	{
		// Left, Right, Top, Bottom
		_cornerPixels = { cornerPixels, cornerPixels, cornerPixels, cornerPixels };

		if (pTexture)
			SetSize(pTexture->w, pTexture->h);
	}

	NineGridImage::NineGridImage(LayoutElement* pParent, Texture* pTexture, std::array<float, 4> corners) : Control(pParent),
		_pTexture { pTexture }
	{
		_cornerPixels = corners;

		if (pTexture)
			SetSize(pTexture->w, pTexture->h);
	}

	void NineGridImage::SetTexture(Texture* pTexture)
	{
		_pTexture = pTexture;
	}

	void NineGridImage::SetCornerSize(float cornerSize)
	{
		_cornerPixels = { cornerSize, cornerSize, cornerSize, cornerSize };
	}

	void NineGridImage::OnRender(Renderer* pRenderer)
	{
		auto rect = GetDrawRect();

		if (_pTexture)
		{
			auto fgColor = GetForegroundColor();

			SDL_SetTextureColorMod(_pTexture, fgColor.r, fgColor.g, fgColor.b);
			SDL_SetTextureAlphaMod(_pTexture, fgColor.a);
			SDL_RenderTexture9Grid(pRenderer, _pTexture, nullptr, _cornerPixels[0], _cornerPixels[1], _cornerPixels[2], _cornerPixels[3], 1.0f, &rect);
		}
	}
}