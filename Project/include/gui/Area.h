#pragma once

#include "Control.h"

/// <summary>
/// Render-less panel
/// </summary>

class Area : public Control
{
public:
	Area(Control* pParent);

protected:
	void OnUpdate(float fDeltaTime) override {};
	void OnRender(SDL_Renderer* pRenderer) override {}
};