#pragma once

#include "Frame.h"

class Panel : public Frame
{
protected:
	void OnRender(SDL_Renderer* pRenderer) override;
};