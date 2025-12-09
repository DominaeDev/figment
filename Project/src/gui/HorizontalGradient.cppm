module;

#include <SDL3/SDL.h>

export module GUI.Controls.HorizontalGradient;
export import GUI.Control;

import Common;
import GUI.GraphicTypes;
import TextureStore;

export
{
	class HorizontalGradient : public Control
	{
	public:
		HorizontalGradient(Control* pParent, Color colorLeft, Color colorRight);
		void SetColors(Color colorLeft, Color colorRight);

	protected:
		void OnUpdate(float fDeltaTime) override {};
		void OnRender(Renderer* pRenderer) override;

		void RefreshGeometry(Rectf rect);
	private:
		Colorf _colorLeft {};
		Colorf _colorRight {};
		Rectf _lastRect {};
		Texture* _pTexture {};

		std::vector<Vertex> _vertices {};
	};
}

HorizontalGradient::HorizontalGradient(Control* pParent, Color colorLeft, Color colorRight) : Control(pParent)
{
	SetColors(colorLeft, colorRight);
	_pTexture = TextureStore::GetTexture(TextureType::BLANK);
}

void HorizontalGradient::SetColors(Color colorLeft, Color colorRight)
{
	_colorLeft = color_util::to_colorf(colorLeft);
	_colorRight = color_util::to_colorf(colorRight);
}

void HorizontalGradient::OnRender(Renderer* pRenderer)
{
	auto bgColor = GetBackgroundColor();
	auto fgColor = GetForegroundColor();

	Rectf rect = GetRect();
	if (!SDL_RectsEqualFloat(&_lastRect, &rect) || _vertices.empty())
		RefreshGeometry(rect);

	static int indices[6] = { 0, 1, 2, 2, 3, 0 };

	SDL_RenderGeometry(pRenderer, _pTexture, _vertices.data(), toI(_vertices.size()), indices, 6);
}

void HorizontalGradient::RefreshGeometry(Rectf rect)
{
	_lastRect = rect;

	_vertices.clear();
	_vertices.reserve(4);

	float left = rect.x;
	float right = rect.x + rect.w;
	float top = rect.y;
	float bottom = rect.y + rect.h;

	_vertices.push_back(Vertex { Pointf { left, bottom }, _colorLeft });
	_vertices.push_back(Vertex { Pointf { right, bottom }, _colorRight });
	_vertices.push_back(Vertex { Pointf { right, top }, _colorRight });
	_vertices.push_back(Vertex { Pointf { left, top }, _colorLeft });
}
