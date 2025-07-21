#pragma once

#include "Control.h"

class Panel : public Control
{
public:
	Panel(Control* pParent);

protected:
	void OnUpdate(float fDeltaTime) override {};
	void OnRender(Renderer* pRenderer) override;
};