#include <pch.h>
#include "gui/NineGridBackgroundRenderer.h"
#include "gui/AppResources.h"
#include "gui/GUIUtility.h"

using namespace fig::gui;
using namespace fig::gui::util;

NineGridBackgroundRenderer::NineGridBackgroundRenderer(int cornerPixels)
{
	_cornerPixels = { toF(cornerPixels), toF(cornerPixels), toF(cornerPixels), toF(cornerPixels)};
}

NineGridBackgroundRenderer::NineGridBackgroundRenderer(std::array<float, 4> corners)
{
	_cornerPixels = corners;
}

void NineGridBackgroundRenderer::Render(Renderer* pRenderer, const Rectf& rect)
{
	auto expandedRect = expand_rect(rect, 5.0f);
	
	if (_pBGTexture)
	{
		SDL_SetTextureColorMod(_pBGTexture, _bgColor.r, _bgColor.g, _bgColor.b);
		SDL_SetTextureAlphaMod(_pBGTexture, _bgColor.a);
		SDL_RenderTexture9Grid(pRenderer, _pBGTexture, nullptr, _cornerPixels[0], _cornerPixels[1], _cornerPixels[2], _cornerPixels[3], _cornerSize / 20.0f, &expandedRect);
	}

	if (_pBorderTexture)
	{
		SDL_SetTextureColorMod(_pBorderTexture, _borderColor.r, _borderColor.g, _borderColor.b);
		SDL_SetTextureAlphaMod(_pBorderTexture, _borderColor.a);
		SDL_RenderTexture9Grid(pRenderer, _pBorderTexture, nullptr, _cornerPixels[0], _cornerPixels[1], _cornerPixels[2], _cornerPixels[3], _cornerSize / 20.0f, &expandedRect);
	}
}

void NineGridBackgroundRenderer::SetColor(Color bgColor)
{
	_bgColor = bgColor;
}

void NineGridBackgroundRenderer::SetTexture(Texture* pBGTexture)
{
	_pBGTexture = pBGTexture;
}

void NineGridBackgroundRenderer::SetColors(Color bgColor, Color borderColor)
{
	_bgColor = bgColor;
	_borderColor = borderColor;
}

void NineGridBackgroundRenderer::SetTextures(Texture* pBGTexture, Texture* pBorderTexture)
{
	_pBGTexture = pBGTexture;
	_pBorderTexture = pBorderTexture;
}

void NineGridBackgroundRenderer::SetCornerSize(float cornerSize)
{
	_cornerSize = cornerSize;
}