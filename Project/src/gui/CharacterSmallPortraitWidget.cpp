#include <pch.h>
#include "gui/CharacterSmallPortraitWidget.h"
#include "gui/AppResources.h"

using namespace fig::io;

namespace fig::gui
{
	CharacterSmallPortraitWidget::CharacterSmallPortraitWidget(ControlPtr pParent) : ImageViewport(pParent), MouseEventHandler(this)
	{
		SetSize(Constants::Data::SmallPortraitWidth, Constants::Data::SmallPortraitHeight);
	}

	void CharacterSmallPortraitWidget::SetImage(const fig::uuid& assetId)
	{
		_loader.Cancel();
		_loader.LoadAsync(assetId,
			[this](AsyncImageLoadResult result) { 
				SetImage(*result);
			});
	}

	void CharacterSmallPortraitWidget::SetImage(const fig::sdl::Surface& surface)
	{
		auto pRenderer = GetSDLRenderer();
		if (auto pTexture = SDL_CreateTextureFromSurface(pRenderer, surface.get()))
		{
			_loader.Cancel();
			_imageTexture.reset(pTexture);
			SetTexture(pTexture);

			if (_imageTexture.get()->w <= Constants::Data::SmallPortraitWidth and _imageTexture.get()->h <= Constants::Data::SmallPortraitHeight)
				GetBorderRenderer()->SetColor(Color::LineColor);
			else
				GetBorderRenderer()->SetColor(0x40C0FF_rgb);
		}
	}

	void CharacterSmallPortraitWidget::OnUpdate(float fElapsed)
	{
		_loader.Poll();
	}

	EventResult CharacterSmallPortraitWidget::OnEvent(fig::event& event)
	{
		if (auto result = ImageViewport::OnEvent(event); result != EventResult::Pass)
			return result;
		return MouseEventHandler::HandleMouseEvents(event);
	}

	fig::sdl::Surface CharacterSmallPortraitWidget::GetImage() const
	{
		if (_targetTexture.empty())
			return {};

		auto pRenderer = GetSDLRenderer();
		fig::sdl::Texture target = fig::sdl::Texture(pRenderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET, Constants::Data::SmallPortraitWidth, Constants::Data::SmallPortraitHeight);

		fig::texture_ptr pLastTarget = SDL_GetRenderTarget(pRenderer);
		if (!SDL_SetRenderTarget(pRenderer, target.get()))
			return {};

		fig::rectf dstRect {
			0.0f,
			0.0f,
			(float)Constants::Data::SmallPortraitWidth,
			(float)Constants::Data::SmallPortraitHeight,
		};

		// Draw background

		SDL_SetRenderDrawColor(pRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
		SDL_RenderClear(pRenderer);
		SDL_RenderTexture(pRenderer, _targetTexture.get(), NULL, &dstRect);

		fig::sdl::Surface surface = fig::sdl::Surface::from_ptr(SDL_RenderReadPixels(pRenderer, NULL));
		SDL_SetRenderTarget(pRenderer, pLastTarget);

		surface.reset(SDL_ConvertSurface(surface.get(), SDL_PIXELFORMAT_RGB24));
		return surface;
	}
}

