#pragma once

#include "VerticalListSizer.h"

class VerticalScrollSizer : public VerticalListSizer
{
public:
	void SetOffset(float offset);

protected:
	void OnLayout(SDL_FRect rect) override;

	float _offset = 0.0f;
};