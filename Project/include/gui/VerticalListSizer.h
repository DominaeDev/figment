#pragma once

#include "Sizer.h"

class VerticalListSizer : public Sizer
{
public:
	void SetSpacing(int spacing) { _spacing = spacing; }
	void SetBottomMargin(int margin) { _marginBottom = margin; }

	float GetListHeight() const { return _totalListHeight; }

protected:
	virtual void OnLayout(SDL_FRect rect) override;

	float _totalListHeight;
	int _marginBottom = 0;
	int _spacing = 0;
};