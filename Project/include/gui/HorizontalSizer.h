#pragma once

#include "Sizer.h"

class HorizontalSizer : public Sizer
{
protected:
	void OnLayout(SDL_FRect rect) override;
};