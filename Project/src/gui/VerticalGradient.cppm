module;

#include <SDL3/SDL.h>

export module GUI.Controls.VerticalGradient;
export import GUI.Control;

import Common;
import GUI.GraphicTypes;
import TextureStore;


export
{
	class VerticalGradient : public Control
	{
	public:
		VerticalGradient(Control* pParent, Color colorTop, Color colorBottom);
		void SetColors(Color colorTop, Color colorBottom);

	protected:
		void OnUpdate(float fDeltaTime) override {};
		void OnRender(Renderer* pRenderer) override;

		void RefreshGeometry(Rectf rect);
	private:
		Colorf _colorTop {};
		Colorf _colorBottom {};
		Rectf _lastRect {};
		Texture* _pTexture {};

		std::vector<Vertex> _vertices {};
	};
}

VerticalGradient::VerticalGradient(Control* pParent, Color colorTop, Color colorBottom) : Control(pParent)
{
	SetColors(colorTop, colorBottom);
	_pTexture = TextureStore::GetTexture(TextureType::BLANK);
}

void VerticalGradient::SetColors(Color colorTop, Color colorBottom)
{
	_colorTop = color_util::to_colorf(colorTop);
	_colorBottom = color_util::to_colorf(colorBottom);
}

void VerticalGradient::OnRender(Renderer* pRenderer)
{
	auto bgColor = GetBackgroundColor();
	auto fgColor = GetForegroundColor();

	Rectf rect = GetRect();
	if (!SDL_RectsEqualFloat(&_lastRect, &rect) || _vertices.empty())
		RefreshGeometry(rect);

	static int indices[6] = { 0, 1, 2, 2, 3, 0 };

	SDL_RenderGeometry(pRenderer, _pTexture, _vertices.data(), toI(_vertices.size()), indices, 6);
}

void VerticalGradient::RefreshGeometry(Rectf rect)
{
	_lastRect = rect;

	_vertices.clear();
	_vertices.reserve(4);

	float left = rect.x;
	float right = rect.x + rect.w;
	float top = rect.y;
	float bottom = rect.y + rect.h;

	_vertices.push_back(Vertex { Pointf { left, bottom }, _colorBottom });
	_vertices.push_back(Vertex { Pointf { right, bottom }, _colorBottom });
	_vertices.push_back(Vertex { Pointf { right, top }, _colorTop });
	_vertices.push_back(Vertex { Pointf { left, top }, _colorTop });
}
