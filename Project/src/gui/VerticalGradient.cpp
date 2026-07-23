#include <pch.h>
#include "gui/VerticalGradient.h"
#include "gui/AppResources.h"

using namespace fig::gui;

VerticalGradient::VerticalGradient(ParentPtr pParent, fig::color colorTop, fig::color colorBottom) : Control(pParent)
{
	SetColors(colorTop, colorBottom);
	_pTexture = AppResources::GetTexture(Resource::BLANK);
}

void VerticalGradient::SetColors(fig::color colorTop, fig::color colorBottom)
{
	_colorTop = to_colorf(colorTop);
	_colorBottom = to_colorf(colorBottom);
}

void VerticalGradient::SetTexture(fig::texture_ptr pTexture)
{
	_pTexture = pTexture;
}

void VerticalGradient::OnRender(fig::renderer_ptr pRenderer)
{
	fig::rectf rect = GetDrawRect();
	if (!SDL_RectsEqualFloat(&_lastRect, &rect) || _vertices.empty())
		RefreshGeometry(rect);

	static constexpr int indices[6] = { 0, 1, 2, 2, 3, 0 };
	SDL_RenderGeometry(pRenderer, _pTexture, _vertices.data(), toI(_vertices.size()), indices, 6);
}

void VerticalGradient::RefreshGeometry(fig::rectf rect)
{
	_lastRect = rect;

	_vertices.clear();
	_vertices.reserve(4);
	
	float left = rect.x;
	float right = rect.x + rect.w;
	float top = rect.y;
	float bottom = rect.y + rect.h;

	_vertices.push_back(fig::vertex { fig::pointf { left, bottom }, _colorBottom });
	_vertices.push_back(fig::vertex { fig::pointf { right, bottom }, _colorBottom });
	_vertices.push_back(fig::vertex { fig::pointf { right, top }, _colorTop });
	_vertices.push_back(fig::vertex { fig::pointf { left, top }, _colorTop });
}
