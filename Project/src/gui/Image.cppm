module;

#include <SDL3/SDL.h>

export module Image;

import Types;
import Control;
import Graphics;
import Color;

export
{
	class Image : public Control
	{
	public:
		Image(Control* pParent, Texture* pTexture);
		void SetTexture(Texture* pTexture);

	protected:
		void OnUpdate(float fDeltaTime) override {};
		void OnRender(Renderer* pRenderer) override;

	private:
		Texture* _pTexture = nullptr;
	};
}

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
		SDL_RenderTexture(pRenderer, _pTexture, nullptr, &rect);
	}
}

void Image::SetTexture(Texture* pTexture)
{
	_pTexture = pTexture;
}