#pragma once

#include "Sizer.h"

class VerticalSizer : public Sizer
{
protected:
	void OnLayout(Rectf rect) override;
};