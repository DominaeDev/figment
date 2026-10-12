#pragma once

#include "gui/Control.h"
#include "gui/MouseEventHandler.h"
#include "io/AsyncIO.h"
#include <future>

namespace fig::gui
{
	using ImageCarouselDelegate = std::function<void(fig::uuid)>;

	class BackgroundImageCarousel : public Control, public MouseEventHandler
	{
	public:
		BackgroundImageCarousel(ControlPtr pParent);

		void LoadCharacterPortraits(fig::uuid characterId);

		void Clear();
		size_t GetImageCount() const noexcept { return _images.size(); }
		void SetDelegate(ImageCarouselDelegate fnDelegate) noexcept { _fnDelegate = fnDelegate; }

	protected:
		bool AddImage(const fig::uuid& assetId, bool bPortrait);

		void RefreshImagePositions();
		fig::coord GetTotalWidth() const noexcept;
		bool IsOverImage() const noexcept;

		void OnUpdate(float fElapsed) override;
		EventResult OnEvent(fig::event& event) override;
		void OnSize() override;

		void OnClickedAt(fig::point pos) override;
		void OnButtonUp(int32_t button) override;
		void OnMouseMotion(fig::point pos) override;
		void OnEnabled(bool bEnabled) override;
	private:
		struct CarouselImage
		{
			fig::uuid assetId; // To original
			fig::sdl::Texture texture;
			fig::io::AsyncFuture future;
			fig::observer_ptr<NonOwningImageWithMask> pControl;
			bool bInside {};
		};
		std::vector<CarouselImage> _images;
		ImageCarouselDelegate _fnDelegate;

		bool _bSliding = false;
		fig::coord _prevSlideOffset {};
		fig::coord _slideOffset {};
	};
}