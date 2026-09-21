#include <pch.h>
#include "gui/HorizontalGradient.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	HorizontalGradient::HorizontalGradient(ControlPtr pParent, fig::color_ref_with_alpha colorLeft, fig::color_ref_with_alpha colorRight) : Control(pParent)
	{
		SetColors(colorLeft, colorRight);
		_pTexture = AppResources::GetTexture(Resource::BLANK);
	}
	
	void HorizontalGradient::SetColors(fig::color_ref_with_alpha colorLeft, fig::color_ref_with_alpha colorRight)
	{
		_colorLeft = colorLeft;
		_colorRight = colorRight;
	}

	void HorizontalGradient::OnRender(fig::renderer_ptr pRenderer)
	{
		auto bgColor = GetBackgroundColor();
		auto fgColor = GetForegroundColor();

		auto& rect = GetRect();
		if (!SDL_RectsEqual(&_lastRect, &rect) || _vertices.empty())
			RefreshGeometry(rect);

		static int indices[6] = { 0, 1, 2, 2, 3, 0 };

		SDL_RenderGeometry(pRenderer, _pTexture, _vertices.data(), toI(_vertices.size()), indices, 6);
	}

	void HorizontalGradient::RefreshGeometry(const fig::rect& rect)
	{
		_lastRect = rect;

		_vertices.clear();
		_vertices.reserve(4);
	
		float left = toF(rect.x);
		float right = toF(rect.x + rect.w);
		float top = toF(rect.y);
		float bottom = toF(rect.y + rect.h);

		_vertices.push_back(fig::vertex { fig::pointf { left, bottom }, _colorLeft });
		_vertices.push_back(fig::vertex { fig::pointf { right, bottom }, _colorRight });
		_vertices.push_back(fig::vertex { fig::pointf { right, top }, _colorRight });
		_vertices.push_back(fig::vertex { fig::pointf { left, top }, _colorLeft });
	}
}
