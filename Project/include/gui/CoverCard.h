#ifndef COVER_CARD_H__
#define COVER_CARD_H__

#pragma once

#include "Image.h"
#include "fs/ImageSource.h"

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

	class CoverCard : public Image, public fig::io::IImageSource
	{
	public:
		CoverCard(Control* pParent, const fig::uuid& assetId);

		void SetBorder(BorderStyle style);
		void SetCoverImage(fig::sdl::Texture texture);

	protected:
		void SetLabel(const fig::string& text) noexcept;
		void SetSublabel(const fig::string& text) noexcept;
		void CreateChatCounter(uint32_t count);
		void AddTag(const fig::string& text, const Color& color);

	private:
		fig::uuid _assetId;
		fig::sdl::Surface _imageSurface {};
		fig::sdl::Texture _imageTexture {};
		fig::sdl::Texture _borderTexture {};
		StaticText* _pLabel {};
		StaticText* _pSublabel {};
		TexturedBorder* _pSimpleBorder {};
		Image* _pStyledBorder {};

		Area* _pFooter {};
		NineGridImage* _pFooterFade {};
		Pointf _tagPosition {};
		int32_t _tagRows { 1 };
	};
}

#endif