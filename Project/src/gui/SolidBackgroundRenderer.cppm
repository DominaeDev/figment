export module SolidBackgroundRenderer;

import <SDL3/SDL.h>;
import Common;
import CustomRenderer;

export
{
	class SolidBackgroundRenderer : public CustomRenderer
	{
	public:
		SolidBackgroundRenderer(Color color);

		void Render(Renderer* pRenderer, Rectf rect);
		void SetColor(Color color);

	private:
		Color _color {};
	};
}

SolidBackgroundRenderer::SolidBackgroundRenderer(Color color)
{
	SetColor(color);
}

void SolidBackgroundRenderer::Render(Renderer* pRenderer, Rectf rect)
{
	SDL_SetRenderDrawColor(pRenderer, _color.r, _color.g, _color.b, SDL_ALPHA_OPAQUE);
	SDL_RenderFillRect(pRenderer, &rect);
}

void SolidBackgroundRenderer::SetColor(Color color)
{
	_color = color;
}