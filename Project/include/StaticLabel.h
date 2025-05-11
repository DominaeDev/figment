#pragma once

#include "Control.h"
#include "Fonts.h"

class StaticLabel : public Control
{
public:
	StaticLabel(const char* text, FontFace fontFace, double ptSize);
	virtual ~StaticLabel();

	void SetText(const char* text);

protected:
	void OnRender(SDL_Renderer* pRenderer) override;
	void OnParent() override;

private:
	void ReleaseTexture();

	const char* _pText = nullptr;
	TTF_Font* _pFont = nullptr;
	SDL_Texture* _pTexture = nullptr;
	SDL_Surface* _pSurface = nullptr;
};