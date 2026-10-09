#pragma once

#include "gui/Control.h"
#include "io/AsyncIO.h"
#include <future>

namespace fig::gui
{
	using ImageCarouselDelegate = std::function<void(fig::uuid)>;

	class ImageCarousel : public Control, public MouseEventHandler
	{
	public:
		ImageCarousel(ControlPtr pParent, fig::coord thumbnailWidth, fig::coord thumbnailHeight);

		void LoadCharacterPortraits(fig::uuid characterId);

		void Clear();
		bool AddImage(const fig::uuid& assetId);
		size_t GetImageCount() const noexcept { return _images.size(); }
		void SetDelegate(ImageCarouselDelegate fnDelegate) noexcept { _fnDelegate = fnDelegate; }

		void ShutUp();

	protected:
		void CenterImages() noexcept;
		void RefreshImagePositions();
		fig::coord GetTotalWidth() const noexcept;
		bool IsOverImage() const noexcept;

		void OnUpdate(float fElapsed) override;
		EventResult OnEvent(fig::event& event) override;
		void OnSize() override;

		void OnMouseEnter() override;
		void OnMouseExit() override;
		void OnClickedAt(fig::point pos) override;
		void OnButtonUp(int32_t button) override;
		void OnMouseMotion(fig::point pos) override;

	private:
		struct CarouselImage
		{
			fig::uuid assetId; // To original
			fig::sdl::Texture texture;
			fig::io::AsyncFuture future;
			fig::observer_ptr<NonOwningImageWithMask> pControl;
			float fAlpha {};
			float fTargetAlpha {};
			bool bInside {};
		};
		std::vector<CarouselImage> _images;
		fig::coord _thumbnailWidth {};
		fig::coord _thumbnailHeight {};
		ImageCarouselDelegate _fnDelegate;

		bool _bHover = false;
		bool _bSliding = false;
		float _fSlidingCounter {};
		float _fHoverCounter {};
		float _fAlpha {};
		fig::coord _prevSlideOffset {};
		fig::coord _slideOffset {};
		float _fShutUpTimer {};
	};
}