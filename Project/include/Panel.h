#pragma once

#include "Control.h"

class Panel : public Control
{
protected:
	void OnUpdate(float fDeltaTime) override {};
	void OnRender(SDL_Renderer* pRenderer) override;
};