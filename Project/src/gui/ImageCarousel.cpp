#include <pch.h>
#include "gui/ImageCarousel.h"
#include "gui/NonOwningImageWithMask.h"
#include "gui/AppResources.h"
#include "gui/TexturedBorderRenderer.h"
#include "io/ContentManager.h"
#include "io/AssetManager.h"

using namespace fig::io;

namespace fig::gui
{
	constexpr fig::coord kImageMargin = 8;
	constexpr fig::coord kImageSpacing = 3;

	ImageCarousel::ImageCarousel(ControlPtr pParent, fig::coord thumbnailWidth, fig::coord thumbnailHeight) : Control(pParent), MouseEventHandler(this),
		_thumbnailWidth { thumbnailWidth },
		_thumbnailHeight { thumbnailHeight }
	{
		SetHeight(70);
		EnableClipping(true);
	}

	void ImageCarousel::LoadCharacterPortraits(fig::uuid characterId)
	{
		Clear();
		auto portraitAssets = Global::GetUserContent().GetAssets().FindAssetsOfType(make_asset_type(AssetType::Image, ImageAssetType::LargePortrait), characterId);
		std::ranges::sort(portraitAssets, std::ranges::less(), [](auto&& a) { return a.get().GetOrder(); });
		for (auto& asset : portraitAssets)
			AddImage(asset.get().id);
	}

	bool ImageCarousel::AddImage(const fig::uuid& assetId)
	{
		auto& content = Global::GetUserContent();
		auto& assets = content.GetAssets();

		if (auto thumbnailAsset = assets.FindAssetOfType(make_asset_type(AssetType::Image, ImageAssetType::Thumbnail), assetId))
		{
			if (auto request = assets.LoadAssetAsync((*thumbnailAsset).id, AsyncTask::LoadImage, 0); request.future.valid())
			{
				auto pWidget = CreateControl<NonOwningImageWithMask>(nullptr, AppResources::GetTexture(Resource::MASK_THUMBNAIL_PORTRAIT));
				pWidget->SetX((_thumbnailWidth + kImageSpacing) * toI(_images.size()));
				pWidget->SetSize(_thumbnailWidth, _thumbnailHeight);
				auto pBorder = pWidget->SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_6PX, 8);
				pBorder->SetColor(Color::Border);

				CarouselImage image {
					.assetId = assetId,
					.future = std::move(request.future),
					.pControl = pWidget,
				};
				
				_images.push_back(std::move(image));
				CenterImages();
				SetEnabled(_images.size() > 1uz);
				return true;
			}
		}
		return false;
	}

	void ImageCarousel::Clear()
	{
		for (int32_t i = toI(_images.size()) - 1; i >= 0; --i)
		{
			auto& image = _images[toUZ(i)];
			DestroyChild(image.pControl);
		}
		_images.clear();
		SetVisible(false);
		_bHover = false;
		_bSliding = false;
	}

	fig::coord ImageCarousel::GetTotalWidth() const noexcept
	{
		if (_images.empty())
			return 0;
		return _thumbnailWidth * toI(_images.size()) + kImageSpacing * (toI(_images.size()) - 1);
	}

	bool ImageCarousel::IsOverImage() const noexcept
	{
		if (not _bHover)
			return false;

		return std::ranges::find_if(_images, [](auto&& i) { return i.bInside; }) != std::ranges::cend(_images);
	}

	void ImageCarousel::OnUpdate(float fElapsed)
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
		
		_bHover |= _bSliding;
		if (_bHover)
			_fHoverCounter = 1.0f;

		float kFadeSpeed = 10.0f;
		float fTargetAlpha = _bHover ? 1.0f : 0.0f;
		step_alpha(_fAlpha, fTargetAlpha, fElapsed * kFadeSpeed);

		if (_bHover)
		{
			if (not GetVisible())
				SetVisible(true);

			auto mpos = GetMousePos();
			for (auto& image : _images)
			{
				auto& widget = *image.pControl.get();
				auto& imageRect = widget.GetRect();
				bool bInside = is_inside(imageRect, mpos);
				image.fTargetAlpha = bInside or _bSliding ? 1.0f : 0.25f;
				image.bInside = bInside;
				step_alpha(image.fAlpha, image.fTargetAlpha, fElapsed * kFadeSpeed * 1.5f);
			}
		}

		for (auto& image : _images)
		{
			image.pControl->SetForegroundColor(opacity(_fAlpha * image.fAlpha));
			image.pControl->GetBorderRenderer()->SetColor(fig::color_ref(Color::Border).WithAlpha(_fAlpha * image.fAlpha));
		}

		if (_bMouseLeftDown and not _bSliding)
		{
			fig::coord maxSlide = std::max(GetTotalWidth() - (GetWidth() - kImageMargin * 2), 0);
			_fSlidingCounter += fElapsed / 0.35f;
			if (_fSlidingCounter >= 1.0f and maxSlide > 0 and IsOverImage())
			{
				_bSliding = true;
				_prevSlideOffset = _slideOffset;
			}
		}

		if (_fHoverCounter > 0.0f)
		{
			if ((_fHoverCounter -= fElapsed) <= 0.0f)
				SetVisible(false);
		}

		if (_fShutUpTimer > 0.0f)
		{
			if ((_fShutUpTimer -= fElapsed) <= 0.0f)
			{
				auto mpos = GetMousePos();
				_bHover = is_inside(GetRect(), mpos.x, mpos.y);
				_bSliding = false;
			}
		}
	}

	EventResult ImageCarousel::OnEvent(fig::event& event)
	{
		if (_fShutUpTimer > 0.0f)
			return EventResult::Pass;

		if (auto result = MouseEventHandler::HandleMouseEvents(event); result == EventResult::Handled)
		{
			if (IsOverImage())
				return EventResult::Handled;
		}

		if (event.type == SDL_EVENT_MOUSE_BUTTON_UP and event.button.button == SDL_BUTTON_LEFT)
		{
			_bHover = is_inside(GetRect(), toI(event.button.x), toI(event.button.y));
			_bSliding = false;
			return EventResult::Continue;
		}

		return EventResult::Pass;
	}

	void ImageCarousel::OnSize()
	{
		_bHover = false;
		_bSliding = false;
		SetVisible(false);
		CenterImages();
	}

	void ImageCarousel::OnMouseEnter()
	{
		if (!_bHover)
			_bHover = true;
	}

	void ImageCarousel::OnMouseExit()
	{
		if (_bHover)
			_bHover = false;
		_fSlidingCounter = 0.0f;
	}

	void ImageCarousel::OnClickedAt(fig::point pos)
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

	void ImageCarousel::OnButtonUp(int32_t button)
	{
		if (button == SDL_BUTTON_LEFT)
		{
			if (_bSliding)
			{
				_fSlidingCounter = 0.0f;
				_bSliding = false;
				return;
			}
		}
	}

	void ImageCarousel::OnMouseMotion(fig::point pos)
	{
		fig::coord maxSlide = std::max((GetTotalWidth() + kImageMargin * 2) - GetWidth(), 0);

		if (not _bSliding and _bMouseLeftDown and maxSlide > 0 and not is_near(pos, _lastLeftDownPos))
		{
			_bSliding = true;
			_prevSlideOffset = _slideOffset;
		}

		if (_bSliding)
		{
			fig::coord maxSlide = std::max((GetTotalWidth() + kImageMargin * 2) - GetWidth(), 0);
			if (maxSlide > 0)
			{
				_slideOffset = _prevSlideOffset + (pos.x - _lastLeftDownPos.x);
				_slideOffset = std::clamp(_slideOffset, -maxSlide, 0);
			}
			RefreshImagePositions();
		}
	}

	void ImageCarousel::CenterImages() noexcept
	{
		_slideOffset = ((GetWidth() - kImageMargin * 2) - GetTotalWidth()) / 2;
		_slideOffset = std::max(_slideOffset, 0);
		RefreshImagePositions();
	}

	void ImageCarousel::RefreshImagePositions()
	{
		for (int32_t i = 0; i < toI(_images.size()); ++i)
		{
			auto& image = _images[i];
			auto& widget = *image.pControl.get();
			widget.SetX(kImageMargin + (_thumbnailWidth + kImageSpacing) * i + _slideOffset);
			widget.SetY(GetHeight() - widget.GetHeight() - kImageMargin);
		}
	}

	void ImageCarousel::ShutUp()
	{
		_fShutUpTimer = 1.5f;
		_bHover = false;
		_bSliding = false;
		DropState();
	}
}