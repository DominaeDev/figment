#include <pch.h>
#include "gui/VerticalBar.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	VerticalBar::VerticalBar(ControlPtr pParent, Resource texture, float fCapSizeV) : IMeshControl(pParent),
		_fCapSizeV { fCapSizeV }
	{
		SetTexture(AppResources::GetTexture(texture));
		if (_pTexture)
			SetSize(_pTexture->w, _pTexture->h);
	}

	void VerticalBar::RefreshGeometry(const fig::rectf& rect)
	{
		float size = rect.w;

		ClearMesh(12, 18);

		auto color = GetForegroundColor();

		// Top cap
		AddPoint(rect.x, rect.y, 0.0f, 0.0f, color);
		AddPoint(rect.x, rect.y + size, 0.0f, _fCapSizeV, color);
		AddPoint(rect.x + rect.w, rect.y + size, 1.0f, _fCapSizeV, color);
		AddPoint(rect.x + rect.w, rect.y, 1.0f, 0.0f, color);
		AddQuad();

		// Bar
		AddPoint(rect.x, rect.y + size, 0.0f, _fCapSizeV, color);
		AddPoint(rect.x, rect.y + rect.h - size, 0.0f, 1.0f - _fCapSizeV, color);
		AddPoint(rect.x + rect.w, rect.y + rect.h - size, 1.0f, 1.0f - _fCapSizeV, color);
		AddPoint(rect.x + rect.w, rect.y + size, 1.0f, _fCapSizeV, color);
		AddQuad();

		// Bottom cap
		AddPoint(rect.x, rect.y + rect.h - size, 0.0f, 1.0f - _fCapSizeV, color);
		AddPoint(rect.x, rect.y + rect.h, 0.0f, 1.0f, color);
		AddPoint(rect.x + rect.w, rect.y + rect.h, 1.0f, 1.0f, color);
		AddPoint(rect.x + rect.w, rect.y + rect.h - size, 1.0f, 1.0f - _fCapSizeV, color);
		AddQuad();
	}

}
