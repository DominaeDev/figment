#include <pch.h>
#include "gui/HorizontalGradient.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	HorizontalGradient::HorizontalGradient(ControlPtr pParent, fig::color_ref_with_alpha colorLeft, fig::color_ref_with_alpha colorRight) : IMeshControl(pParent)
	{
		SetColors(colorLeft, colorRight);
	}

	void HorizontalGradient::SetColors(fig::color_ref_with_alpha colorLeft, fig::color_ref_with_alpha colorRight)
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
