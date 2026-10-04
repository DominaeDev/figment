#include <pch.h>
#include "gui/ModalOverlay.h"
#include "gui/Frame.h"
#include "gui/AppResources.h"
#include "gui/TexturedBorder.h"
#include "gui/CustomRenderers.h"
#include "gui/MenuSeparator.h"
#include "app/AppState.h"
#include "gui/Events.h"
#include <fast_gaussian_blur_template.h>

namespace fig::gui
{
	constexpr float fBlurSigma = 2.5f;
	constexpr float fBlurOpacity = 0.5f;

	ModalOverlay::ModalOverlay(FramePtr pFrame) : Overlay(pFrame)
	{
		SetSize(pFrame->GetSize());
		SetVisible(false);
	}

	int32_t ModalOverlay::Show()
	{
		_overlayId = _pOwner->PushModal(this);
		return _overlayId;
	}

	void ModalOverlay::Close()
	{
		_pOwner->PopModal(this);
	}

	void ModalOverlay::BlurBackground()
	{
		_future = _pOwner->GetSnapshot();
	}

	void ModalOverlay::OnUpdate(float fElapsed)
	{
		if (_future.valid() and _future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
		{
			if (auto result = _future.get(); not result.empty())
			{
				_pBGTexture.reset();
				if (_pBGImage)
					DestroyChild(_pBGImage);

				_pBGImage = CreateControl<Image>(_pBGTexture.get());
				_pBGImage->SetSize(GetSize());
				_pBGImage->SetVisible(false);
				_pBGImage->SetForegroundColor(Color::Transparent);
				_pBGTint = CreateControl<Panel>();
				_pBGTint->SetSize(GetSize());
				_pBGTint->SetBackgroundColor(Color::Transparent);
				MoveChildToBottom(_pBGTint);
				MoveChildToBottom(_pBGImage);

				_bWaitOnBlur = true;
				_pBlurResult.store(nullptr);
				_blurWorker = std::jthread(std::bind_front(&ModalOverlay::Blur, this), std::move(result), fBlurSigma);
			}
		}

		if (_bWaitOnBlur)
		{
			if (auto pResult = _pBlurResult.load())
			{
				_pBlurResult.store(nullptr);
				_bWaitOnBlur = false;
				_fBlurFade = 0.0f;
				_pBGTexture.reset(SDL_CreateTextureFromSurface(GetSDLRenderer(), pResult));
				_pBGImage->SetTexture(_pBGTexture.get());
				_pBGImage->SetVisible(true);
				_pBGImage->SetForegroundColor(Color::Transparent);
				SDL_DestroySurface(pResult);

				SetVisible(true);
			}
		}
		else
		{
			if (_pBGImage && _pBGImage->HasTexture() and _fBlurFade < 1.0f)
			{
				_fBlurFade += fElapsed / 0.3f;
				if (_fBlurFade > 1.0f)
					_fBlurFade = 1.0f;
				
				float fInverse = 1.0f - _fBlurFade;
				float fEaseOut = 1.0f - fInverse * fInverse;
				_pBGImage->SetForegroundColor(fig::color_ref(Color::White).WithAlpha(fEaseOut));
				_pBGTint->SetBackgroundColor(fig::color_ref(Color::CardShadow).WithAlpha(fBlurOpacity * fEaseOut));
			}
		}
	}

	void ModalOverlay::OnSize()
	{
		if (_pBGImage)
			_pBGImage->SetSize(GetSize());
		if (_pBGTint)
			_pBGTint->SetSize(GetSize());
	}

	void ModalOverlay::Blur(fig::sdl::Surface&& surface, float sigma)
	{
		if (surface.empty() or sigma <= 0.0f)
			return;

		auto pSurface = surface.get();

		// Rescale image
		constexpr int32_t MaxSize = 768;
		fig::point size { surface->w, surface->h };
		if (size.x > MaxSize or size.y > MaxSize)
		{
			float scale = std::min(toF(MaxSize) / size.x, toF(MaxSize) / size.y);
			size.x = static_cast<int32_t>(toF(size.x) * scale);
			size.y = static_cast<int32_t>(toF(size.y) * scale);

			pSurface = SDL_ScaleSurface(pSurface, size.x, size.y, SDL_SCALEMODE_NEAREST);
		}

		// Blur
		if (SDL_LockSurface(pSurface))
		{
			auto components = pSurface->pitch / pSurface->w;
			auto pixels = static_cast<unsigned char*>(pSurface->pixels);
			size_t length = pSurface->w * pSurface->h * components;

			std::vector<unsigned char> new_pixels(length);
			unsigned char* new_pixel_data = new_pixels.data();
			fast_gaussian_blur(pixels, new_pixel_data, pSurface->w, pSurface->h, components, sigma, 2U, kExtend);
			std::memcpy(pixels, new_pixels.data(), length);
			SDL_UnlockSurface(pSurface);
		}
		
		surface.release();
		_pBlurResult.store(pSurface);
	}
}