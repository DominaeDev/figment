#pragma once

#include "gui/ImageViewport.h"
#include "io/AsyncImageLoad.h"

namespace fig::gui
{
	class PreviewCardImage;

	class CharacterSmallPortraitWidget : public ImageViewport, public MouseEventHandler
	{
	public:
		CharacterSmallPortraitWidget(ControlPtr pParent);

		void SetImage(const fig::uuid& assetId);
		void SetImage(const fig::sdl::Surface& surface);

		fig::sdl::Surface GetImage() const;

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