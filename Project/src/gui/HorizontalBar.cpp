#include <pch.h>
#include "gui/HorizontalBar.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	HorizontalBar::HorizontalBar(ControlPtr pParent, Resource texture, float fCapSizeU) : IMeshControl(pParent),
		_fCapSizeU { fCapSizeU }
	{
		SetTexture(AppResources::GetTexture(texture));
		if (_pTexture)
			SetSize(_pTexture->w, _pTexture->h);
	}

	void HorizontalBar::RefreshGeometry(const fig::rectf& rect)
	{
		float size = rect.h;

		ClearMesh(12, 18);

		auto color = GetForegroundColor();

		// Left cap
		AddPoint(rect.x, rect.y, 0.0f, 0.0f, color);
		AddPoint(rect.x, rect.y + rect.h, 0.0f, 1.0f, color);
		AddPoint(rect.x + size, rect.y + rect.h, _fCapSizeU, 1.0f, color);
		AddPoint(rect.x + size, rect.y, _fCapSizeU, 0.0f, color);
		AddQuad();

		// Bar
		AddPoint(rect.x + size, rect.y, _fCapSizeU, 0.0f, color);
		AddPoint(rect.x + size, rect.y + rect.h, _fCapSizeU, 1.0f, color);
		AddPoint(rect.x + rect.w - size, rect.y + rect.h, 1.0f - _fCapSizeU, 1.0f, color);
		AddPoint(rect.x + rect.w - size, rect.y, 1.0f - _fCapSizeU, 0.0f, color);
		AddQuad();

		// Right cap
		AddPoint(rect.x + rect.w - size, rect.y, 1.0f - _fCapSizeU, 0.0f, color);
		AddPoint(rect.x + rect.w - size, rect.y + rect.h, 1.0f - _fCapSizeU, 1.0f, color);
		AddPoint(rect.x + rect.w, rect.y + rect.h, 1.0f, 1.0f, color);
		AddPoint(rect.x + rect.w, rect.y, 1.0f, 0.0f, color);
		AddQuad();
	}

}
