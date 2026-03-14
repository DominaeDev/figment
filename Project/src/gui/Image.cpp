#include <pch.h>
#include "gui/Image.h"
#include "gui/GUIUtility.h"

using namespace fig::gui;
using namespace fig::gui::util;

Image::Image(LayoutElement* pParent, Texture* pTexture, Color tint) : Control(pParent),
	_pTexture(pTexture)
{
	if (pTexture)
		SetSize(toF(pTexture->w), toF(pTexture->h));
	
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
		Rectf rect = GetRect();

		if (fgColor.IsDefined())
			SDL_SetTextureColorMod(_pTexture, fgColor.r, fgColor.g, fgColor.b);
		else
			SDL_SetTextureColorMod(_pTexture, 0xFF, 0xFF, 0xFF);

		if (fgColor.IsDefined() && fgColor.a != 0)
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