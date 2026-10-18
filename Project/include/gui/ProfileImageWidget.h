#pragma once

#include "gui/ImageViewport.h"
#include "gui/MouseEventHandler.h"
#include "io/AsyncImageLoad.h"

namespace fig::gui
{
	class PreviewCardImage;

	class ProfileImageWidget : public ImageViewport, public MouseEventHandler
	{
	public:
		ProfileImageWidget(ControlPtr pParent);

		void SetImage(const fig::sdl::Surface& surface);
		fig::sdl::Surface GetImage(fig::coord width, fig::coord height) const;

		void SetEditable(bool bEditable);
		
	protected:
		void OnUpdate(float fElapsed) override;
		EventResult OnEvent(fig::event& event) override;
		void OnMouseEnter() override;
		void OnMouseExit() override;

		fig::io::AsyncImageLoad _loader {};
		fig::sdl::Texture _imageTexture {};

		bool _bEditable {};
		bool _bHasError {};
	};
}