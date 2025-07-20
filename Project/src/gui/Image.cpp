#include "gui/Image.h"
#include "gui/Color.h"

Image::Image(Control* pParent, SDL_Texture* pTexture) : Control(pParent),
	_pTexture(pTexture)
{
}

void Image::OnRender(SDL_Renderer* pRenderer)
{
	auto bgColor = GetBackgroundColor();
	auto fgColor = GetForegroundColor();
	if (Color::IsDefined(bgColor) && bgColor.a != 0)
		DrawBackground(pRenderer);

	if (_pTexture)
	{
		SDL_FRect rect = GetRect();

		if (Color::IsDefined(fgColor) && fgColor.a != 0)
			SDL_SetTextureAlphaMod(_pTexture, fgColor.a);
		else
			SDL_SetTextureAlphaMod(_pTexture, 0xFF);
		SDL_RenderTexture(pRenderer, _pTexture, NULL, &rect);
	}
}

void Image::SetTexture(SDL_Texture* pTexture)
{
	_pTexture = pTexture;
}