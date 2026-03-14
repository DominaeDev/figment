#ifndef COVER_CARD_H__
#define COVER_CARD_H__

#pragma once

#include "Image.h"
#include "model/AssetManager.h"
#include "util/Dictionary.h"

namespace fig::gui
{
	enum CardBorderStyle
	{
		None,
		Style01,
		Style02,
		Style03,
		Style04,
		Style05,
		Style06,
	};

	enum class CardSize
	{
		Default,
		Half,
	};

	class TexturedBorder;
	class NineGridImage;

	class CoverCard : public Image
	{
	public:
		CoverCard(LayoutElement* pParent, const fig::uuid& assetId, CardSize cardSize = CardSize::Default);

		void SetBorder(CardBorderStyle style);
		void SetCardSize(CardSize cardSize);
		void SetPendingCoverImage(fig::io::AsyncFuture&& future);
		bool IsFilteredBy(const fig::string& search_string) const noexcept;

	protected:
		void SetCoverImages(fig::sdl::Surface&& surface, fig::sdl::Surface&& half);

		void SetLabel(const fig::string& text) noexcept;
		void CreateChatCounter(uint32_t count);

		enum class AddTagResult { Ok, Reject, Stop };
		AddTagResult AddTag(const fig::string& tag);
		AddTagResult AddTag(const fig::string& tag, const Color& color);

		void OnUpdate(float fElapsed) override;
		void OnRender(Renderer* pRenderer) override;
		void OnSize() override;
		void AddSearchText(const fig::string& text) noexcept;
		void AddSearchText(const std::span<fig::string> texts) noexcept;
		void AddSearchText(const fig::wstring& text) noexcept;
		void AddSearchText(const std::span<fig::wstring> texts) noexcept;

	private:
		void PollFuture();
		void RefreshImage();

	private:
		fig::uuid _assetId;
		TexturedBorder* _pSimpleBorder {};
		Image* _pStyledBorder {};
		CardSize _cardSize {};
		Control* _pCounterBG;

		Area* _pLargeFooter {};
		NineGridImage* _pLargeFooterFade {};
		StaticText* _pLabel {};
		Pointf _tagPosition {};
		int32_t _tagRows { 1 };

		Area* _pSmallFooter {};
		StaticText* _pSmallLabel {};

		fig::sdl::Surface _imageSurface {};
		fig::sdl::Texture _imageTexture {};
		fig::sdl::Surface _smallImageSurface {};
		fig::sdl::Texture _smallImageTexture {};
		fig::io::AsyncFuture _pendingCover {};

		bool _bHasError = false;
		Image* _pErrorIcon {};

		std::vector<fig::wstring> _searchWords {};
		std::set<fig::string> _tags {};

	};
}

#endif