#ifndef VERTICAL_SCROLL_SIZER_H__
#define VERTICAL_SCROLL_SIZER_H__

#include "VerticalListSizer.h"

class VerticalScrollSizer : public VerticalListSizer
{
public:
	void SetOffset(float offset);

protected:
	void OnLayout(Rectf rect) override;

	float _offset = 0.0f;
};

#endif