#include <pch.h>
#include "gui/Border.h"
#include "gui/TextureStore.h"
#include "gui/GUIUtility.h"

using namespace fig::gui;
using namespace fig::gui_util;

Border::Border(Control* pParent, Texture* borderTexture, int cornerPixels) : Control(pParent)
{
	_cornerPixels = { toF(cornerPixels), toF(cornerPixels), toF(cornerPixels), toF(cornerPixels) };
	_pBorderTexture = borderTexture;
}

Border::Border(Control* pParent, Texture* borderTexture, std::array<float, 4> corners) : Control(pParent)
{
	_cornerPixels = corners;
	_pBorderTexture = borderTexture;
}

void Border::OnRender(Renderer* pRenderer)
{
	if (_pBorderTexture)
	{
		auto fgColor = GetForegroundColor();
		Rectf rect = GetRect();
		SDL_SetTextureColorMod(_pBorderTexture, fgColor.r, fgColor.g, fgColor.b);
		SDL_SetTextureAlphaMod(_pBorderTexture, fgColor.a);
		SDL_RenderTexture9Grid(pRenderer, _pBorderTexture, nullptr, _cornerPixels[0], _cornerPixels[1], _cornerPixels[2], _cornerPixels[3], _cornerSize / 20.0f, &rect);
	}
}

void Border::SetColors(Color bgColor, Color borderColor)
{
	_bgColor = bgColor;
	_borderColor = borderColor;
}

void Border::SetTexture(Texture* pBorderTexture)
{
	_pBorderTexture = pBorderTexture;
}

void Border::SetCornerSize(float cornerSize)
{
	_cornerSize = cornerSize;
}