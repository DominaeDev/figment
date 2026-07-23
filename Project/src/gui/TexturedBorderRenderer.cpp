#include <pch.h>
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"
#include "gui/GUIUtility.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	TexturedBorderRenderer::TexturedBorderRenderer(Resource borderTexture, int32_t cornerPixels) : CustomRenderer()
	{
		_cornerPixels = { toF(cornerPixels), toF(cornerPixels), toF(cornerPixels), toF(cornerPixels) };
		SetTexture(AppResources::GetTexture(borderTexture));
	}

	TexturedBorderRenderer::TexturedBorderRenderer(Resource borderTexture, std::array<int32_t, 4> cornerPixels) : CustomRenderer()
	{
		_cornerPixels = { toF(cornerPixels[0]), toF(cornerPixels[1]), toF(cornerPixels[2]), toF(cornerPixels[3]) };
		SetTexture(AppResources::GetTexture(borderTexture));
	}

	TexturedBorderRenderer::TexturedBorderRenderer(fig::texture_ptr borderTexture, int32_t cornerPixels) : CustomRenderer()
	{
		_cornerPixels = { toF(cornerPixels), toF(cornerPixels), toF(cornerPixels), toF(cornerPixels) };
	}

	TexturedBorderRenderer::TexturedBorderRenderer(fig::texture_ptr borderTexture, std::array<int32_t, 4> cornerPixels) : CustomRenderer()
	{
		_cornerPixels = { toF(cornerPixels[0]), toF(cornerPixels[1]), toF(cornerPixels[2]), toF(cornerPixels[3]) };
	}

	void TexturedBorderRenderer::SetExtend(float size)
	{
		_fExtend = std::max(size, 0.0f);
	}

	void TexturedBorderRenderer::Render(fig::renderer_ptr pRenderer, const fig::rectf& rect)
	{
		auto expandedRect = expand_rect(rect, _fExtend);

		if (_pTexture)
		{
			SDL_SetTextureColorMod(_pTexture, _color.r, _color.g, _color.b);
			SDL_SetTextureAlphaMod(_pTexture, _color.a);
			SDL_RenderTexture9Grid(pRenderer, _pTexture, nullptr, _cornerPixels[0], _cornerPixels[1], _cornerPixels[2], _cornerPixels[3], _fCornerScale, &expandedRect);
		}
	}

	void TexturedBorderRenderer::SetTexture(fig::texture_ptr pTexture)
	{
		_pTexture = pTexture;
	}

	void TexturedBorderRenderer::SetCornerScale(float cornerSize)
	{
		_fCornerScale = cornerSize;
	}
}