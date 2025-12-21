#include <pch.h>
#include "gui/Image.h"
#include "gui/GUIUtility.h"

using namespace fig::gui;
using namespace fig::gui_util;

Image::Image(Control* pParent, Texture* pTexture) : Control(pParent),
	_pTexture(pTexture)
{
}

void Image::OnRender(Renderer* pRenderer)
{
	auto bgColor = GetBackgroundColor();
	auto fgColor = GetForegroundColor();
	if (is_defined(bgColor) && bgColor.a != 0)
		DrawBackground(pRenderer);

	if (_pTexture)
	{
		Rectf rect = GetRect();

		if (is_defined(fgColor) && fgColor.a != 0)
			SDL_SetTextureAlphaMod(_pTexture, fgColor.a);
		else
			SDL_SetTextureAlphaMod(_pTexture, 0xFF);
		SDL_RenderTexture(pRenderer, _pTexture, NULL, &rect);
	}
}

void Image::SetTexture(Texture* pTexture)
{
	_pTexture = pTexture;
}