#include <pch.h>
#include "gui/ChatBackground.h"

namespace fig::gui
{
	ChatBackground::ChatBackground(ParentPtr parent) : Control(parent)
	{
		EnableClipping(true);
	}

	void ChatBackground::SetTexture(TexturePtr pTexture) noexcept
	{
		_pTexture = pTexture;

		if (_pTexture)
		{
			_imageSize = Point { _pTexture->w, _pTexture->h };
			_fImageRatio = toF(_imageSize.x) / toF(_imageSize.y);
		}
		else
		{
			_imageSize = Point {};
			_fImageRatio = 1.0f;
		}
	}

	void ChatBackground::SetBrightness(float alpha)
	{
		_value = static_cast<uint8_t>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f);
	}

	void ChatBackground::SetBrightness(uint8_t alpha)
	{
		_value = alpha;
	}

	void ChatBackground::SetAlpha(float alpha)
	{
		_alpha = static_cast<uint8_t>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f);
	}

	void ChatBackground::SetAlpha(uint8_t alpha)
	{
		_alpha = alpha;
	}

	Rectf ChatBackground::GetImageRect() const
	{
		float dstWidth = toF(GetWidth());
		float dstHeight = toF(GetHeight());
		float srcWidth = toF(_imageSize.x);
		float srcHeight = toF(_imageSize.y);
		float scale;

		if (_fit == ImageFit::Inside)
			scale = std::min(toF(dstWidth) / srcWidth, toF(dstHeight) / srcHeight);
		else if (_fit == ImageFit::Outside || _fit == ImageFit::Portrait)
			scale = std::max(toF(dstWidth) / srcWidth, toF(dstHeight) / srcHeight);
		else
			scale = 1.0f;

		Rectf drawRect;
		drawRect.w = srcWidth * scale;
		drawRect.h = srcHeight * scale;
		drawRect.x = (dstWidth - drawRect.w) / 2.0f;
		if (_fit == ImageFit::Portrait)
			drawRect.y = 0.0f;
		else
			drawRect.y = (dstHeight - drawRect.h) / 2.0f;
		drawRect.x += GetRect().x;
		drawRect.y += GetRect().y;

		if (flt_eq(scale, 1.0f))
		{
			// Round to nearest pixel
			drawRect.x = std::roundf(drawRect.x);
			drawRect.y = std::roundf(drawRect.y);
			drawRect.w = std::roundf(drawRect.w);
			drawRect.h = std::roundf(drawRect.h);
		}

		return drawRect;
	}

	void ChatBackground::SetImage(const fig::uuid& assetId)
	{
		if (auto try_image = Global::GetUserContent().GetTexture(assetId, GetSDLRenderer()))
			SetTexture((*try_image).get());
	}

	void ChatBackground::OnRender(RendererPtr pRenderer)
	{
		if (!(bool)_pTexture)
			return;

		Color bg = _backgroundColor;
		bg.Add(255 - _backgroundColor.a);

		auto drawRect = GetImageRect();
		SDL_SetTextureBlendMode(_pTexture, SDL_BLENDMODE_BLEND);
		SDL_SetTextureColorMod(_pTexture, _value, _value, _value);
		SDL_SetTextureAlphaMod(_pTexture, _alpha);
		SDL_RenderTexture(pRenderer, _pTexture, NULL, &drawRect);
	}
}