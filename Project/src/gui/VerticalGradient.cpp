#include "gui/VerticalGradient.h"
#include "gui/Color.h"
#include "gui/TextureStore.h"
#include "util/Utility.h"

VerticalGradient::VerticalGradient(Control* pParent, SDL_Color colorTop, SDL_Color colorBottom) : Control(pParent)
{
	SetColors(colorTop, colorBottom);
	_pTexture = TextureStore::GetTexture(Texture::BLANK);
}

void VerticalGradient::SetColors(SDL_Color colorTop, SDL_Color colorBottom)
{
	_colorTop = Color::ColorToFColor(colorTop);
	_colorBottom = Color::ColorToFColor(colorBottom);
}

void VerticalGradient::OnRender(SDL_Renderer* pRenderer)
{
	auto bgColor = GetBackgroundColor();
	auto fgColor = GetForegroundColor();

	Rect rect = GetRect();
	if (!SDL_RectsEqualFloat(&_lastRect, &rect) || _vertices.empty())
		RefreshGeometry(rect);

	static int indices[6] = { 0, 1, 2, 2, 3, 0 };

	SDL_RenderGeometry(pRenderer, _pTexture, _vertices.data(), toI(_vertices.size()), indices, 6);
}

void VerticalGradient::RefreshGeometry(SDL_FRect rect)
{
	_lastRect = rect;

	_vertices.clear();
	_vertices.reserve(4);
	
	float left = rect.x;
	float right = rect.x + rect.w;
	float top = rect.y;
	float bottom = rect.y + rect.h;

	_vertices.push_back(SDL_Vertex { SDL_FPoint { left, bottom }, _colorBottom });
	_vertices.push_back(SDL_Vertex { SDL_FPoint { right, bottom }, _colorBottom });
	_vertices.push_back(SDL_Vertex { SDL_FPoint { right, top }, _colorTop });
	_vertices.push_back(SDL_Vertex { SDL_FPoint { left, top }, _colorTop });
}
