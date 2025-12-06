#ifndef VERTICAL_SIZER_H__
#define VERTICAL_SIZER_H__

#pragma once

#include "Sizer.h"

class VerticalSizer : public Sizer
{
protected:
	void OnLayout(Rectf rect) override;
};
#endif