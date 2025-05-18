#pragma once

#include "Sizer.h"

class VerticalListSizer : public Sizer
{
public:
	void SetSpacing(int spacing) { _spacing = spacing; }
	void SetBottomMargin(int margin) { _marginBottom = margin; }

protected:
	void OnLayout(SDL_FRect rect) override;

	int _marginBottom = 0;
	int _spacing = 0;
};