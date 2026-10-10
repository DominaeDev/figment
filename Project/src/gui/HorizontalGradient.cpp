#include <pch.h>
#include "gui/HorizontalGradient.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	HorizontalGradient::HorizontalGradient(ControlPtr pParent, fig::color_ref colorLeft, fig::color_ref colorRight) : IMeshControl(pParent)
	{
		SetColors(colorLeft, colorRight);
		int k = 0;
	}

	void HorizontalGradient::SetColors(fig::color_ref colorLeft, fig::color_ref colorRight)
	{
		_colorLeft = colorLeft;
		_colorRight = colorRight;
		InvalidateMesh();
	}

	void HorizontalGradient::RefreshGeometry(const fig::rectf& rect)
	{
		ClearMesh(4, 6);

		float left = rect.x;
		float right = rect.x + rect.w;
		float top = rect.y;
		float bottom = rect.y + rect.h;

		AddPoint(left, bottom, _colorLeft);
		AddPoint(right, bottom, _colorRight);
		AddPoint(right, top, _colorRight);
		AddPoint(left, top, _colorLeft);
		AddQuad();
	}
}
