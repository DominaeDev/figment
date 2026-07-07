#include <pch.h>
#include "gui/NineGridImage.h"

namespace fig::gui
{
	NineGridImage::NineGridImage(LayoutElement* pParent, TexturePtr pTexture, Coord cornerSize) : Control(pParent),
		_pTexture { pTexture }
	{
		// Left, Right, Top, Bottom
		_cornerPixels = std::array<float, 4> { toF(cornerSize), toF(cornerSize), toF(cornerSize), toF(cornerSize) };

		if (pTexture)
			SetSize(pTexture->w, pTexture->h);
	}

	NineGridImage::NineGridImage(LayoutElement* pParent, TexturePtr pTexture, Corners corners) : Control(pParent),
		_pTexture { pTexture }
	{
		_cornerPixels = std::array<float, 4> { toF(corners[0]), toF(corners[1]), toF(corners[2]), toF(corners[3]) };

		if (pTexture)
			SetSize(pTexture->w, pTexture->h);
	}

	void NineGridImage::SetTexture(TexturePtr pTexture)
	{
		_pTexture = pTexture;
	}

	void NineGridImage::SetCornerSize(Coord cornerSize)
	{
		_cornerPixels = std::array<float, 4> { toF(cornerSize), toF(cornerSize), toF(cornerSize), toF(cornerSize) };
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