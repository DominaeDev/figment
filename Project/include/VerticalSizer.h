#pragma once

#include "Sizer.h"

class VerticalSizer : public Sizer
{
protected:
	void OnLayout(SDL_FRect rect) override;
};