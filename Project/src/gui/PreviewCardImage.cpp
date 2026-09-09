#include <pch.h>
#include "gui/PreviewCardImage.h"
#include "gui/AppResources.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/GUIUtility.h"
#include "io/ContentManager.h"

using namespace fig::io;

namespace fig::gui
{
	PreviewCardImage::PreviewCardImage(ControlPtr pParent, ImageFit fit) : Control(pParent),
		_fit { fit }
	{
		SetSize(Constants::GUI::Cards::Half::Width, Constants::GUI::Cards::Half::Height);

		SetForegroundColor(Color::White);
		SetBackgroundColor(Color::Transparent);

		SetMask(AppResources::GetTexture(Resource::MASK_CARD));

		auto pBorder = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_6PX, 8);
		pBorder->SetColor(Color::LineColor);
	}

	bool PreviewCardImage::SetImage(const fig::uuid& assetId)
	{
		if (auto request = Global::GetUserContent().GetAssets().LoadAssetAsync(assetId, AsyncTask::LoadImage, 0); request.future.valid())
		{
			SetPendingCoverImage(std::move(request.future));
			return true;
		}
		return false;
	}

	void PreviewCardImage::OnUpdate(float fElapsed)
	{
		if (_imageTexture.empty())
			PollFuture();
	}

	void PreviewCardImage::OnRender(fig::renderer_ptr pRenderer)
	{
		auto bgColor = GetBackgroundColor();
		auto fgColor = GetForegroundColor();
		if (bgColor.IsDefined() && bgColor.a != 0)
			DrawBackground(pRenderer);

		if (_bRedraw)
		{
			Redraw();
			_bRedraw = false;
			_bRedrawAlpha = false;
		}

		if (auto pTexture = _targetTexture.get())
		{
			auto rect = GetDrawRect();

			if (fgColor.IsDefined())
				SDL_SetTextureColorMod(pTexture, fgColor.r, fgColor.g, fgColor.b);
			else
				SDL_SetTextureColorMod(pTexture, 0xFF, 0xFF, 0xFF);

			if (fgColor.IsDefined() && fgColor.a != 0)
				SDL_SetTextureAlphaMod(pTexture, fgColor.a);
			else
				SDL_SetTextureAlphaMod(pTexture, 0xFF);

			SDL_RenderTexture(pRenderer, pTexture, NULL, &rect);
		}
	}

	void PreviewCardImage::SetPendingCoverImage(AsyncFuture&& future)
	{
		if (not future.valid())
			return;

		_pendingRequest = std::move(future);
		PollFuture();
	}

	void PreviewCardImage::PollFuture()
	{
		if (not _pendingRequest.valid())
			return;

		if (auto try_surface = GetAsyncResult<fig::sdl::Surface>(_pendingRequest))
		{
			auto pRenderer = GetSDLRenderer();
			if (auto pTexture = SDL_CreateTextureFromSurface(pRenderer, (**try_surface).get()))
			{
				_imageTexture.reset(pTexture);
				OnTexture();
			}
		}
		else if (try_surface.error() != AsyncLoadError::NoError)
		{
			// Error
		}
	}

	void PreviewCardImage::OnTexture() noexcept
	{
		if (not _imageTexture.empty())
			_imageSize = fig::point { _imageTexture->w, _imageTexture->h };
		else
			_imageSize = fig::point {};

		SetDirty();
	}

	void PreviewCardImage::SetMask(fig::texture_ptr pMask) noexcept
	{
		_pMask = pMask;
		_bRedraw = true;
		_bRedrawAlpha = true;
	}

	void PreviewCardImage::Redraw()
	{
		auto width = std::min(GetWidth(), 2048);
		auto height = std::min(GetHeight(), 2048);

		auto pRenderer = GetSDLRenderer();
		SDL_assert(pRenderer);

		fig::texture_ptr pTarget = _targetTexture.get();
		if (!pTarget)
		{
			pTarget = SDL_CreateTexture(pRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
			_targetTexture.reset(pTarget);
			_bRedrawAlpha = true;
		}

		auto priorRenderTarget = SDL_GetRenderTarget(pRenderer);
		SDL_SetRenderTarget(pRenderer, pTarget);

		fig::rectf drawRect = to_rectf(ScaleToFit(rect { 0, 0, _imageSize.x, _imageSize.y }, rect { 0, 0, width, height }, _fit));

		// Render with alpha
		if (_pMask)
		{
			if (_bRedrawAlpha)
			{
				SDL_SetRenderDrawColor(pRenderer, 0, 0, 0, 0);
				SDL_RenderClear(pRenderer);
				SDL_SetTextureBlendMode(_pMask, SDL_BLENDMODE_NONE);
				SDL_RenderTexture9Grid(pRenderer, _pMask, nullptr, 8, 8, 8, 8, 1.0f, NULL);
			}

			SDL_BlendMode blendMode = SDL_ComposeCustomBlendMode(
				SDL_BLENDFACTOR_SRC_ALPHA,
				SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
				SDL_BLENDOPERATION_ADD,
				SDL_BLENDFACTOR_ZERO,
				SDL_BLENDFACTOR_ONE,
				SDL_BLENDOPERATION_ADD);

			// Background color
			constexpr auto bgColor = Color::LineColor;
			SDL_SetRenderDrawBlendMode(pRenderer, blendMode);
			SDL_SetRenderDrawColor(pRenderer, bgColor.r, bgColor.g, bgColor.b, 255);
			SDL_RenderFillRect(pRenderer, NULL);
			SDL_SetRenderDrawBlendMode(pRenderer, SDL_BLENDMODE_BLEND);

			// Render texture
			if (not _imageTexture.empty())
			{
				auto pTexture = _imageTexture.get();
				SDL_SetTextureBlendMode(pTexture, blendMode);
				SDL_SetTextureColorMod(pTexture, 0xFF, 0xFF, 0xFF);
				SDL_SetTextureAlphaMod(pTexture, 0xFF);
				SDL_RenderTexture(pRenderer, pTexture, NULL, &drawRect);
			}
		}
		else if (not _imageTexture.empty())
		{
			auto pTexture = _imageTexture.get();
			SDL_SetTextureBlendMode(pTexture, SDL_BLENDMODE_BLEND);
			SDL_SetTextureColorMod(pTexture, 0xFF, 0xFF, 0xFF);
			SDL_SetTextureAlphaMod(pTexture, 0xFF);
			SDL_RenderTexture(pRenderer, pTexture, NULL, &drawRect);
		}

		SDL_SetRenderTarget(pRenderer, priorRenderTarget);
	}

	void PreviewCardImage::SetDirty()
	{
		_bRedraw = true;
	}

	void PreviewCardImage::OnSize()
	{
		SetDirty();
	}
}