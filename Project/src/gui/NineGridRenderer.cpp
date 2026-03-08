#include <pch.h>
#include "gui/NineGridRenderer.h"
#include "gui/AppResources.h"
#include "gui/GUIUtility.h"

using namespace fig::gui::util;

namespace fig::gui
{
	NineGridRenderer::NineGridRenderer(int cornerPixels)
	{
		_cornerPixels = { toF(cornerPixels), toF(cornerPixels), toF(cornerPixels), toF(cornerPixels) };
	}

	NineGridRenderer::NineGridRenderer(std::array<float, 4> corners)
	{
		_cornerPixels = corners;
	}

	void NineGridRenderer::SetExtend(float size)
	{
		_fExtend = std::max(size, 0.0f);
	}

	void NineGridRenderer::Render(Renderer* pRenderer, const Rectf& rect)
	{
		auto expandedRect = expand_rect(rect, _fExtend);

		if (_pBGTexture)
		{
			SDL_SetTextureColorMod(_pBGTexture, _bgColor.r, _bgColor.g, _bgColor.b);
			SDL_SetTextureAlphaMod(_pBGTexture, _bgColor.a);
			SDL_RenderTexture9Grid(pRenderer, _pBGTexture, nullptr, _cornerPixels[0], _cornerPixels[1], _cornerPixels[2], _cornerPixels[3], _fCornerSize / 20.0f, &expandedRect);
		}

		if (_pBorderTexture)
		{
			SDL_SetTextureColorMod(_pBorderTexture, _borderColor.r, _borderColor.g, _borderColor.b);
			SDL_SetTextureAlphaMod(_pBorderTexture, _borderColor.a);
			SDL_RenderTexture9Grid(pRenderer, _pBorderTexture, nullptr, _cornerPixels[0], _cornerPixels[1], _cornerPixels[2], _cornerPixels[3], _fCornerSize / 20.0f, &expandedRect);
		}
	}

	void NineGridRenderer::SetColor(Color bgColor)
	{
		_bgColor = bgColor;
	}

	void NineGridRenderer::SetTexture(Texture* pBGTexture)
	{
		_pBGTexture = pBGTexture;
	}

	void NineGridRenderer::SetCornerSize(float cornerSize)
	{
		_fCornerSize = cornerSize;
	}
}