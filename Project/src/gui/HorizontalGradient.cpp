#include "gui/HorizontalGradient.h"
#include "gui/Color.h"
#include "gui/TextureStore.h"

HorizontalGradient::HorizontalGradient(Control* pParent, SDL_Color colorLeft, SDL_Color colorRight) : Control(pParent)
{
	SetColors(colorLeft, colorRight);
	_pTexture = TextureStore::GetTexture(Texture::BLANK);
}

void HorizontalGradient::SetColors(SDL_Color colorLeft, SDL_Color colorRight)
{
	_colorLeft = color_util::to_colorf(colorLeft);
	_colorRight = color_util::to_colorf(colorRight);
}

void HorizontalGradient::OnRender(SDL_Renderer* pRenderer)
{
	auto bgColor = GetBackgroundColor();
	auto fgColor = GetForegroundColor();

	Rect rect = GetRect();
	if (!SDL_RectsEqualFloat(&_lastRect, &rect) || _vertices.empty())
		RefreshGeometry(rect);

	static int indices[6] = { 0, 1, 2, 2, 3, 0 };

	SDL_RenderGeometry(pRenderer, _pTexture, _vertices.data(), toI(_vertices.size()), indices, 6);
}

void HorizontalGradient::RefreshGeometry(SDL_FRect rect)
{
	_lastRect = rect;

	_vertices.clear();
	_vertices.reserve(4);
	
	float left = rect.x;
	float right = rect.x + rect.w;
	float top = rect.y;
	float bottom = rect.y + rect.h;

	_vertices.push_back(SDL_Vertex { SDL_FPoint { left, bottom }, _colorLeft });
	_vertices.push_back(SDL_Vertex { SDL_FPoint { right, bottom }, _colorRight });
	_vertices.push_back(SDL_Vertex { SDL_FPoint { right, top }, _colorRight });
	_vertices.push_back(SDL_Vertex { SDL_FPoint { left, top }, _colorLeft });
}
