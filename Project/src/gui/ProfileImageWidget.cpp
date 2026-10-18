#include <pch.h>
#include "gui/ProfileImageWidget.h"
#include "gui/AppResources.h"
#include "gui/TexturedBorderRenderer.h"

using namespace fig::io;

namespace fig::gui
{
	ProfileImageWidget::ProfileImageWidget(ControlPtr pParent) : ImageViewport(pParent, false), MouseEventHandler(this)
	{
		SetSize(120, 120);
	}

	void ProfileImageWidget::SetImage(const fig::sdl::Surface& surface)
	{
		auto pRenderer = GetSDLRenderer();
		if (auto pTexture = SDL_CreateTextureFromSurface(pRenderer, surface.get()))
		{
			_loader.Cancel();
			_imageTexture.reset(pTexture);
			SetTexture(pTexture);

			if (_imageTexture.get()->w <= Constants::Data::ProfileImageWidth and _imageTexture.get()->h <= Constants::Data::ProfileImageHeight)
				SetEditable(false);
			else
				SetEditable(true);
		}
	}

	void ProfileImageWidget::SetEditable(bool bEditable)
	{
		_bEditable = bEditable;
		if (bEditable)
		{
			auto pBorder = SetBorderRenderer<TexturedBorderRenderer>(Resource::THICK_ROUNDED_BORDER_6PX, 8);
			pBorder->SetColor(Color::SelectedControlBorder);
		}
		else
		{
			auto pBorder = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_6PX, 8);
			pBorder->SetColor(Color::Border);
		}
	}

	void ProfileImageWidget::OnUpdate(float fElapsed)
	{
		_loader.Poll();
	}

	EventResult ProfileImageWidget::OnEvent(fig::event& event)
	{
		if (_bEditable)
		{
			if (auto result = ImageViewport::OnEvent(event); result == EventResult::Handled)
				return result;
		}
		return MouseEventHandler::HandleMouseEvents(event);
	}

	fig::sdl::Surface ProfileImageWidget::GetImage(fig::coord width, fig::coord height) const
	{
		if (not _pTexture)
			return {};

		auto pRenderer = GetSDLRenderer();
		fig::sdl::Texture target = fig::sdl::Texture(pRenderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET, width, height);

		fig::texture_ptr pLastTarget = SDL_GetRenderTarget(pRenderer);
		if (!SDL_SetRenderTarget(pRenderer, target.get()))
			return {};

		float fScale = toF(width) / GetWidth();
		fig::rectf drawRect = CalcDrawRect(width, height, _fZoom, fig::point(toI(_offset.x * fScale), toI(_offset.y * fScale)));

		// Draw background
		SDL_SetRenderDrawColor(pRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
		SDL_RenderClear(pRenderer);
		SDL_SetTextureBlendMode(_pTexture, SDL_BLENDMODE_BLEND);
		SDL_SetTextureColorMod(_pTexture, 0xFF, 0xFF, 0xFF);
		SDL_SetTextureAlphaMod(_pTexture, 0xFF);
		SDL_RenderTexture(pRenderer, _pTexture, NULL, &drawRect);

		fig::sdl::Surface surface = fig::sdl::Surface::from_ptr(SDL_RenderReadPixels(pRenderer, NULL));
		SDL_SetRenderTarget(pRenderer, pLastTarget);

		surface.reset(SDL_ConvertSurface(surface.get(), SDL_PIXELFORMAT_RGB24));
		return surface;
	}

	void ProfileImageWidget::OnMouseEnter()
	{
		if (_bEditable)
			PushEvent(UserEvent::PushCursor, Cursor::Move);
	}

	void ProfileImageWidget::OnMouseExit()
	{
		PushEvent(UserEvent::PopCursor, Cursor::Move);
	}
}

