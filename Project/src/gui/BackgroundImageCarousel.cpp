#include <pch.h>
#include "gui/BackgroundImageCarousel.h"
#include "gui/NonOwningImageWithMask.h"
#include "gui/AppResources.h"
#include "gui/TexturedBorderRenderer.h"
#include "io/ContentManager.h"
#include "io/AssetManager.h"

using namespace fig::io;

namespace fig::gui
{
	constexpr fig::coord kThumbnailSpacing = 4;
	constexpr fig::coord kThumbnailWidth = 50;
	constexpr fig::coord kThumbnailHeight = 64;

	BackgroundImageCarousel::BackgroundImageCarousel(ControlPtr pParent) : Control(pParent), MouseEventHandler(this)
	{
		SetHeight(64);
		EnableClipping(true);
	}

	void BackgroundImageCarousel::LoadCharacterPortraits(fig::uuid characterId)
	{
		auto backgroundAssets = Global::GetUserContent().GetAssets().FindAssetsOfType(make_asset_type(AssetType::Image, ImageAssetType::Background), characterId);
		std::ranges::sort(backgroundAssets, std::ranges::less(), [](auto&& a) { return a.get().GetOrder(); });
		for (auto& asset : backgroundAssets)
			AddImage(asset.get().id, false);

		auto portraitAssets = Global::GetUserContent().GetAssets().FindAssetsOfType(make_asset_type(AssetType::Image, ImageAssetType::LargePortrait), characterId);
		std::ranges::sort(portraitAssets, std::ranges::less(), [](auto&& a) { return a.get().GetOrder(); });
		for (auto& asset : portraitAssets)
			AddImage(asset.get().id, true);
	}

	bool BackgroundImageCarousel::AddImage(const fig::uuid& assetId, bool bPortrait)
	{
		auto& content = Global::GetUserContent();
		auto& assets = content.GetAssets();

		if (auto thumbnailAsset = assets.FindAssetOfType(make_asset_type(AssetType::Image, ImageAssetType::Thumbnail), assetId))
		{
			if (auto request = assets.LoadAssetAsync((*thumbnailAsset).id, AsyncTask::LoadImage, 0); request.future.valid())
			{
				auto rightMost = _images.empty() ? 0 : _images.back().pControl->GetRight();

				auto pWidget = CreateControl<NonOwningImageWithMask>(nullptr, AppResources::GetTexture(Resource::MASK_THUMBNAIL_PORTRAIT));
				pWidget->SetSize(bPortrait ? kThumbnailWidth : kThumbnailHeight, bPortrait ? kThumbnailHeight : kThumbnailWidth);
				pWidget->SetX(rightMost + kThumbnailSpacing);
				pWidget->SetY((GetHeight() - pWidget->GetHeight()) / 2);
				pWidget->SetForegroundColor(GetEnabled() ? Color::White : Color::DarkGray);
				auto pBorder = pWidget->SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_6PX, 8);
				pBorder->SetColor(Color::Border);

				CarouselImage image {
					.assetId = assetId,
					.future = std::move(request.future),
					.pControl = pWidget,
				};

				_images.push_back(std::move(image));
				SetEnabled(_images.size() > 1uz);
				return true;
			}
		}
		return false;
	}

	void BackgroundImageCarousel::Clear()
	{
		for (int32_t i = toI(_images.size()) - 1; i >= 0; --i)
		{
			auto& image = _images[toUZ(i)];
			DestroyChild(image.pControl);
		}
		_images.clear();
		SetVisible(false);
		_bSliding = false;
		_slideOffset = 0;
	}

	fig::coord BackgroundImageCarousel::GetTotalWidth() const noexcept
	{
		if (_images.empty())
			return 0;
		return _images.back().pControl->GetRight() - _images.front().pControl->GetX();
	}

	bool BackgroundImageCarousel::IsOverImage() const noexcept
	{
		if (_images.empty())
			return false;

		auto firstRect = _images.front().pControl->GetRect();
		auto lastRect = _images.back().pControl->GetRect();
		fig::rect rect {
			firstRect.x,
			firstRect.y,
			lastRect.x + lastRect.w - firstRect.x,
			lastRect.h,
		};
		auto thisRect = GetRect();
		SDL_GetRectIntersection(&rect, &thisRect, &rect);
		return is_inside(rect, GetMousePos());
	}

	void BackgroundImageCarousel::OnUpdate(float fElapsed)
	{
		if (not GetEnabled())
			return;

		for (int32_t i = toI(_images.size()) - 1; i >= 0; --i)
		{
			auto& image = _images[toUZ(i)];
			if (image.future.valid())
			{
				if (auto try_surface = GetAsyncResult<fig::sdl::Surface>(image.future))
				{
					auto pTexture = SDL_CreateTextureFromSurface(GetSDLRenderer(), (*try_surface)->get());
					if (pTexture)
					{
						image.texture.reset(pTexture);
						image.pControl->SetTexture(image.texture.get());
					}
				}
				else
				{
					// Error
					DestroyChild(image.pControl);
					_images.erase(_images.cbegin() + toUZ(i));
				}
			}
		}

		if (not GetVisible())
			SetVisible(true);

		auto mpos = GetMousePos();
		for (auto& image : _images)
		{
			auto& widget = *image.pControl.get();
			auto& imageRect = widget.GetRect();
			bool bInside = is_inside(imageRect, mpos);
			image.bInside = bInside;
		}
	}

	EventResult BackgroundImageCarousel::OnEvent(fig::event& event)
	{
		if (auto result = MouseEventHandler::HandleMouseEvents(event); result == EventResult::Handled)
		{
			if (IsOverImage())
				return EventResult::Handled;
		}

		if (event.type == SDL_EVENT_MOUSE_BUTTON_UP and event.button.button == SDL_BUTTON_LEFT)
		{
			_bSliding = false;
			return EventResult::Continue;
		}

		return EventResult::Pass;
	}

	void BackgroundImageCarousel::OnSize()
	{
		_bSliding = false;
		SetVisible(false);
	}

	void BackgroundImageCarousel::OnClickedAt(fig::point pos)
	{
		if (_bSliding or not is_near(pos, _lastLeftDownPos))
			return;

		if (_fnDelegate)
		{
			for (auto& image : _images)
			{
				if (image.bInside and _fnDelegate)
				{
					_fnDelegate(image.assetId);
					return;
				}
			}
		}
	}

	void BackgroundImageCarousel::OnButtonUp(int32_t button)
	{
		if (button == SDL_BUTTON_LEFT)
			_bSliding = false;
	}

	void BackgroundImageCarousel::OnMouseMotion(fig::point pos)
	{
		fig::coord maxSlide = std::max(GetTotalWidth() - GetWidth(), 0);

		if (not _bSliding and _bMouseLeftDown and maxSlide > 0 and not is_near(pos, _lastLeftDownPos))
		{
			_bSliding = true;
			_prevSlideOffset = _slideOffset;
		}

		if (_bSliding)
		{
			fig::coord maxSlide = std::max(GetTotalWidth() - GetWidth() + 10, 0);
			if (maxSlide > 0)
			{
				_slideOffset = _prevSlideOffset + (pos.x - _lastLeftDownPos.x);
				_slideOffset = std::clamp(_slideOffset, -maxSlide, 0);
			}
			RefreshImagePositions();
		}
	}

	void BackgroundImageCarousel::RefreshImagePositions()
	{
		fig::coord x = 0;
		for (auto& image : _images)
		{
			auto& widget = *image.pControl.get();
			widget.SetX(x + _slideOffset);
			widget.SetY((GetHeight() - widget.GetHeight()) / 2);
			x += widget.GetWidth() + kThumbnailSpacing;
		}
	}

	void BackgroundImageCarousel::OnEnabled(bool bEnabled)
	{
		for (auto& image : _images)
		{
			auto& widget = *image.pControl.get();
			widget.SetForegroundColor(bEnabled ? Color::White : Color::DarkGray);
		}
	}
}