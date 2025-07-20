#pragma once

#include "Control.h"
#include "Graphics.h"

class Image : public Control
{
public:
	Image(Control* pParent, SDL_Texture* pTexture);
	void SetTexture(SDL_Texture* pTexture);

protected:
	void OnUpdate(float fDeltaTime) override {};
	void OnRender(SDL_Renderer* pRenderer) override;

private:
	SDL_Texture* _pTexture = nullptr;
};