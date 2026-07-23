#include <pch.h>
#include "gui/RenderTargetControl.h"

namespace fig::gui
{
	RenderTargetControl::RenderTargetControl(ParentPtr parent) : Control(parent)
	{
		SetBackgroundColor(Colors::Transparent);
		_bLocalFromOrigin = true;
	}

	void RenderTargetControl::Render(fig::renderer_ptr pRenderer)
	{
		if (not _bVisible or _bCulled)
			return;

		auto width = std::min(GetWidth(), 8192);
		auto height = std::min(GetHeight(), 8192);

		SDL_assert(pRenderer);

		if (_targetTexture.empty() or _lastSize.x != width or _lastSize.y != height)
		{
			_lastSize = fig::point { width, height };
			fig::texture_ptr pTarget = SDL_CreateTexture(pRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
			if (pTarget)
			{
				SDL_SetTextureBlendMode(pTarget, SDL_BLENDMODE_BLEND);
				SDL_SetTextureColorMod(pTarget, 0xFF, 0xFF, 0xFF);
				_targetTexture.reset(pTarget);
			}
			else
			{
				_targetTexture.clear();
				SetVisible(false);
				return; // Error
			}
		}

		// Draw this
		OnRender(pRenderer);

		auto pTexture = _targetTexture.get();
		auto priorRenderTarget = SDL_GetRenderTarget(pRenderer);

		SDL_SetRenderTarget(pRenderer, pTexture);

		// Clear
		SDL_SetRenderDrawColor(pRenderer, 0x00, 0x00, 0x00, 0x00);
		SDL_RenderClear(pRenderer);

		// Draw children
		fig::rect cullingRect = expand_rect(GetRect(), 64);
		for (auto& child : _children)
		{
			auto renderable = dynamic_cast<Control*>(child); //! @todo: remove this cast
			if (renderable)
			{
				if (_bCulling)
				{
					auto childRect = renderable->GetRect();
					bool bVisible = SDL_HasRectIntersection(&cullingRect, &childRect);
					if (child->IsCulled() == bVisible)
						child->Cull(!bVisible);
				}

				renderable->Render(pRenderer);
			}
		}

		OnRenderMask(pRenderer, _targetTexture);

		SDL_SetRenderTarget(pRenderer, priorRenderTarget);

		// Render target texture
		auto rect = GetDrawRect();
		SDL_SetTextureBlendMode(pTexture, SDL_BLENDMODE_BLEND_PREMULTIPLIED);
		SDL_SetTextureColorMod(pTexture, _alpha, _alpha, _alpha);
		SDL_SetTextureAlphaMod(pTexture, _alpha);
		SDL_RenderTexture(pRenderer, pTexture, NULL, &rect);
		
		DrawBorder(pRenderer);
		OnPostRender();
	}

	void RenderTargetControl::SetAlpha(float alpha)
	{
		_alpha = static_cast<uint8_t>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f);
	}

	void RenderTargetControl::SetAlpha(uint8_t alpha)
	{
		_alpha = alpha;
	}
}