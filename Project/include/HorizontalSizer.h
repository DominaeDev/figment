#pragma once

#include "Sizer.h"

class HorizontalSizer : public Sizer
{
	void Layout(SDL_FRect parentRect) override;
};