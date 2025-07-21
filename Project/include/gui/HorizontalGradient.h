#pragma once

#include "Control.h"
#include "Graphics.h"

class HorizontalGradient : public Control
{
public:
	HorizontalGradient(Control* pParent, SDL_Color colorLeft, SDL_Color colorRight);
	void SetColors(SDL_Color cocolorLeftlorTop, SDL_Color colorRight);

protected:
	void OnUpdate(float fDeltaTime) override {};
	void OnRender(SDL_Renderer* pRenderer) override;

	void RefreshGeometry(SDL_FRect rect);
private:
	SDL_FColor _colorLeft {};
	SDL_FColor _colorRight {};
	SDL_FRect _lastRect {};
	SDL_Texture* _pTexture {};

	std::vector<SDL_Vertex> _vertices {};
};