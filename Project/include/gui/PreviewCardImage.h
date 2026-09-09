#pragma once

#include "gui/Control.h"
#include "io/AssetManager.h"

namespace fig::gui
{
	class PreviewCardImage : public Control
	{
	public:
		PreviewCardImage(ControlPtr pParent, ImageFit fit = ImageFit::Outside);

		bool SetImage(const fig::uuid& assetId);
		void SetMask(fig::texture_ptr pMask) noexcept;

	protected:
		void SetPendingCoverImage(fig::io::AsyncFuture&& future);
		void PollFuture();

		void OnUpdate(float fElapsed);
		void OnRender(fig::renderer_ptr pRenderer) override;
		void OnSize() override;
		void OnTexture() noexcept;

		void Redraw();
		void SetDirty();

	private:
		fig::io::AsyncFuture _pendingRequest {};

		bool _bRedraw = true;
		bool _bRedrawAlpha = true;
		fig::point _imageSize {};
		ImageFit _fit {};

		fig::sdl::Texture _targetTexture {};
		fig::sdl::Texture _imageTexture {};
		fig::texture_ptr _pMask {};
	};
}