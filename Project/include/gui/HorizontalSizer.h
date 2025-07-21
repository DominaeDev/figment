#pragma once

#include "Sizer.h"

class HorizontalSizer : public Sizer
{
protected:
	void OnLayout(Rectf rect) override;
};