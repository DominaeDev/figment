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
		SetSize(Constants::GUI::CharacterEditor::PortraitWidth, Constants::GUI::CharacterEditor::PortraitHeight);

		SetForegroundColor(Colour::White);
		SetBackgroundColor(Colour::White);
		SetBackgroundTexture(AppResources::GetTexture(Resource::CARD_BACKGROUND_DEFAULT));
		SetMask(AppResources::GetTexture(Resource::MASK_CARD));

		auto pBorder = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_6PX, 8);
		pBorder->SetColor(Colour::LineColor);
	}

	void PreviewCardImage::SetImage(const fig::uuid& assetId)
	{
		_loader.LoadAsync(assetId,
			[&](AsyncImageLoadResult result) {
				auto pRenderer = GetSDLRenderer();
				if (auto pTexture = SDL_CreateTextureFromSurface(pRenderer, (*result).get()))
				{
					_imageTexture.reset(pTexture);
					OnTexture();
				}
			}, 
			[&](AsyncLoadError error) {
				_bHasError = true;
			});
	}

	void PreviewCardImage::SetImage(const fig::sdl::Surface& surface)
	{
		if (auto pTexture = SDL_CreateTextureFromSurface(GetSDLRenderer(), surface.get()))
			_imageTexture = fig::sdl::Texture::from_ptr(pTexture);

		OnTexture();
	}

	fig::point PreviewCardImage::GetImageSize() const noexcept
	{
		if (not _imageTexture.empty())
			return fig::point { _imageTexture->w, _imageTexture->h };
		return {};
	}

	void PreviewCardImage::OnUpdate(float fElapsed)
	{
		if (_imageTexture.empty())
			_loader.Poll();

		if (_bHasError && !_pErrorIcon)
		{
			// Create error icon
			constexpr float fScale = 0.75f;
			_pErrorIcon = CreateControl<Image>(AppResources::GetTexture(Resource::ICON_ERROR));
			_pErrorIcon->SetSize(toI(_pErrorIcon->GetTextureSize().x * fScale), toI(_pErrorIcon->GetTextureSize().y * fScale));
			_pErrorIcon->SetForegroundColor(custom_color(0xC0C0C0_rgb));
			_pErrorIcon->Center();
			_pErrorBG = AppResources::GetTexture(Resource::CARD_BACKGROUND_EMPTY);
			SetDirty();
		}
	}

	void PreviewCardImage::OnRender(fig::renderer_ptr pRenderer)
	{
		auto bgColor = GetBackgroundColor();
		auto fgColor = GetForegroundColor();

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
				SDL_SetTextureColorMod(pTexture, fgColor->r, fgColor->g, fgColor->b);
			else
				SDL_SetTextureColorMod(pTexture, 0xFF, 0xFF, 0xFF);

			if (fgColor.IsDefined() && fgColor->a != 0)
				SDL_SetTextureAlphaMod(pTexture, fgColor->a);
			else
				SDL_SetTextureAlphaMod(pTexture, 0xFF);

			SDL_RenderTexture(pRenderer, pTexture, NULL, &rect);
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

			// Render background
			if (_pBGTexture)
			{
				SDL_SetTextureBlendMode(_pBGTexture, blendMode);
				SDL_SetTextureColorMod(_pBGTexture, 0xFF, 0xFF, 0xFF);
				SDL_SetTextureAlphaMod(_pBGTexture, 0xFF);
				SDL_RenderTexture(pRenderer, _pBGTexture, NULL, NULL);
			}
			else
			{
				auto bgColor = GetBackgroundColor();
				SDL_SetRenderDrawBlendMode(pRenderer, blendMode);
				SDL_SetRenderDrawColor(pRenderer, bgColor->r, bgColor->g, bgColor->b, 255);
				SDL_RenderFillRect(pRenderer, NULL);
				SDL_SetRenderDrawBlendMode(pRenderer, SDL_BLENDMODE_BLEND);
			}

			// Render texture
			if (not _imageTexture.empty())
			{
				auto pTexture = _imageTexture.get();
				SDL_SetTextureBlendMode(pTexture, blendMode);
				SDL_SetTextureColorMod(pTexture, 0xFF, 0xFF, 0xFF);
				SDL_SetTextureAlphaMod(pTexture, 0xFF);
				SDL_SetTextureScaleMode(pTexture, SDL_SCALEMODE_LINEAR);
				SDL_RenderTexture(pRenderer, pTexture, NULL, &drawRect);
			}
			else if (_pErrorBG)
			{
				auto pTexture = _pErrorBG.get();
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
			SDL_SetTextureScaleMode(pTexture, SDL_SCALEMODE_LINEAR);
			SDL_RenderTexture(pRenderer, pTexture, NULL, &drawRect);
		}
		else if (_pErrorBG)
		{
			auto pTexture = _pErrorBG.get();
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

	void PreviewCardImage::SetBackgroundTexture(fig::texture_ptr pBGTexture) noexcept
	{
		_pBGTexture = pBGTexture;
		SetDirty();
	}
}