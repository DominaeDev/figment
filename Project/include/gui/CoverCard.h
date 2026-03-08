#ifndef COVER_CARD_H__
#define COVER_CARD_H__

#pragma once

#include "Image.h"
#include "model/AssetManager.h"

namespace fig::gui
{
	enum BorderStyle
	{
		None,
		Style01,
		Style02,
		Style03,
		Style04,
		Style05,
		Style06,
	};

	class TexturedBorder;
	class NineGridImage;

	class CoverCard : public Image
	{
	public:
		CoverCard(LayoutElement* pParent, const fig::uuid& assetId);

		void SetBorder(BorderStyle style);
		void SetCoverImage(fig::sdl::Surface&& texture);
		void SetPendingCoverImage(fig::io::ImageFuture&& future);

	protected:
		void SetLabel(const fig::string& text) noexcept;
		void SetSublabel(const fig::string& text) noexcept;
		void CreateChatCounter(uint32_t count);
		void AddTag(const fig::string& text, const Color& color);

		void OnUpdate(float fElapsed) override;
		void OnRender(Renderer* pRenderer) override;

	private:
		void PollFuture();

	private:
		fig::uuid _assetId;
		StaticText* _pLabel {};
		StaticText* _pSublabel {};
		TexturedBorder* _pSimpleBorder {};
		Image* _pStyledBorder {};

		Area* _pFooter {};
		NineGridImage* _pFooterFade {};
		Pointf _tagPosition {};
		int32_t _tagRows { 1 };
		
		fig::sdl::Surface _imageSurface {};
		fig::sdl::Texture _imageTexture {};
		fig::io::ImageFuture _pendingCover {};

		bool _bHasError = false;
		Image* _pErrorIcon {};
	};
}

#endif