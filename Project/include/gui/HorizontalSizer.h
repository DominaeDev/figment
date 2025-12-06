#ifndef HORIZONTAL_SIZER_H__
#define HORIZONTAL_SIZER_H__

#include "Sizer.h"

class HorizontalSizer : public Sizer
{
protected:
	void OnLayout(Rectf rect) override;
};

#endif