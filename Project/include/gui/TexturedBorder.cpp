#include <pch.h>
#include "gui/TexturedBorder.h"
#include "gui/GUIUtility.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	TexturedBorder::TexturedBorder(ControlPtr pParent, fig::texture_ptr borderTexture, int cornerPixels) : Control(pParent)
	{
		_cornerPixels = { toF(cornerPixels), toF(cornerPixels), toF(cornerPixels), toF(cornerPixels) };
		_pBorderTexture = borderTexture;
	}

	TexturedBorder::TexturedBorder(ControlPtr pParent, fig::texture_ptr borderTexture, std::array<float, 4> corners) : Control(pParent)
	{
		_cornerPixels = corners;
		_pBorderTexture = borderTexture;
	}

	TexturedBorder::TexturedBorder(ControlPtr pParent, Resource borderTexture, int cornerPixels) : Control(pParent)
	{
		_cornerPixels = { toF(cornerPixels), toF(cornerPixels), toF(cornerPixels), toF(cornerPixels) };
		_pBorderTexture = AppResources::GetTexture(borderTexture);
	}

	TexturedBorder::TexturedBorder(ControlPtr pParent, Resource borderTexture, std::array<float, 4> corners) : Control(pParent)
	{
		_cornerPixels = corners;
		_pBorderTexture = AppResources::GetTexture(borderTexture);
	}

	void TexturedBorder::OnRender(fig::renderer_ptr pRenderer)
	{
		if (_pBorderTexture)
		{
			auto fgColor = GetForegroundColor();
			fig::rectf rect = GetDrawRect();
			SDL_SetTextureColorMod(_pBorderTexture, fgColor.r, fgColor.g, fgColor.b);
			SDL_SetTextureAlphaMod(_pBorderTexture, fgColor.a);
			SDL_RenderTexture9Grid(pRenderer, _pBorderTexture, nullptr, _cornerPixels[0], _cornerPixels[1], _cornerPixels[2], _cornerPixels[3], _cornerScale, &rect);
		}
	}

	void TexturedBorder::SetColors(fig::color bgColor, fig::color borderColor)
	{
		_bgColor = bgColor;
		_borderColor = borderColor;
	}

	void TexturedBorder::SetTexture(fig::texture_ptr pBorderTexture)
	{
		_pBorderTexture = pBorderTexture;
	}

	void TexturedBorder::SetCornerScale(float cornerScale)
	{
		_cornerScale = cornerScale;
	}
}