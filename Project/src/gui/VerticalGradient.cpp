#include <pch.h>
#include "gui/VerticalGradient.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	VerticalGradient::VerticalGradient(ControlPtr pParent, fig::color_ref_with_alpha colorTop, fig::color_ref_with_alpha colorBottom) : IMeshControl(pParent)
	{
		SetColors(colorTop, colorBottom);
	}

	void VerticalGradient::SetColors(fig::color_ref_with_alpha colorTop, fig::color_ref_with_alpha colorBottom)
	{
		_colorTop = colorTop;
		_colorBottom = colorBottom;
		InvalidateMesh();
	}

	void VerticalGradient::RefreshGeometry(const fig::rectf& rect)
	{
		ClearMesh(4, 6);

		float left = rect.x;
		float right = rect.x + rect.w;
		float top = rect.y;
		float bottom = rect.y + rect.h;

		AddPoint(left, bottom, _colorBottom);
		AddPoint(right, bottom, _colorBottom);
		AddPoint(right, top, _colorTop);
		AddPoint(left, top, _colorTop);
		AddQuad();
	}
}