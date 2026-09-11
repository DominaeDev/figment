#pragma once

#include "gui/Control.h"
#include "io/AsyncImageLoad.h"

namespace fig::gui
{
	class PreviewCardImage : public Control
	{
	public:
		PreviewCardImage(ControlPtr pParent, ImageFit fit = ImageFit::Outside);

		void SetImage(const fig::uuid& assetId);
		void SetImage(const fig::sdl::Surface& pSurface);
		void SetMask(fig::texture_ptr pMask) noexcept;
		
		fig::point GetImageSize() const noexcept;
	protected:
		void OnUpdate(float fElapsed);
		void OnRender(fig::renderer_ptr pRenderer) override;
		void OnSize() override;
		void OnTexture() noexcept;

		void Redraw();
		void SetDirty();

	private:
		fig::io::AsyncImageLoad _loader {};

		bool _bRedraw = true;
		bool _bRedrawAlpha = true;
		bool _bHasError = false;
		fig::point _imageSize {};
		ImageFit _fit {};

		fig::sdl::Texture _targetTexture {};
		fig::sdl::Texture _imageTexture {};
		fig::texture_ptr _pMask {};

		fig::observer_ptr<Image> _pErrorIcon;
		fig::texture_ptr _pErrorBG;
	};
}