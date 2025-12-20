#include "gui/Image.h"
#include "gui/Color.h"

using namespace fig::gui;

Image::Image(Control* pParent, Texture* pTexture) : Control(pParent),
	_pTexture(pTexture)
{
}

void Image::OnRender(Renderer* pRenderer)
{
	auto bgColor = GetBackgroundColor();
	auto fgColor = GetForegroundColor();
	if (color_util::is_defined(bgColor) && bgColor.a != 0)
		DrawBackground(pRenderer);

	if (_pTexture)
	{
		Rectf rect = GetRect();

		if (color_util::is_defined(fgColor) && fgColor.a != 0)
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