#ifndef COVER_CARD_H__
#define COVER_CARD_H__

#pragma once

#include "Image.h"
#include "model/AssetManager.h"
#include "util/SearchIndex.h"

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
		void Init();

		void SetBorder(CardBorderStyle style);
		void SetCardSize(CardSize cardSize);
		void SetPendingCoverImage(fig::io::AsyncFuture&& future);
		void EnableTags(bool bEnable);
		
		bool IsFilteredBy(const fig::string& search_string) const noexcept;
		const fig::uuid& GetAssetID() const { return _assetId; }

	protected:
		void SetCoverImages(fig::sdl::Surface&& surface, fig::sdl::Surface&& half);

		void SetLabel(const fig::string& text) noexcept;
		void CreateChatCounter(uint32_t count);

		enum class AddTagResult { Ok, Reject, Stop };
		AddTagResult AddTag(const fig::string& tag, const Color& color = {});

		void OnUpdate(float fElapsed) override;
		void OnRender(Renderer* pRenderer) override;
		void OnSize() override;
		
		void SetIndex(const SearchIndex& index) noexcept;
		void AddSearchTerms(const fig::string& text) noexcept;
		void AddSearchTerms(std::span<const fig::string> texts) noexcept;

	private:
		void PollFuture();
		void RefreshImage();
		void CreatePendingTags();
		void CreatePendingLabel();

	private:
		fig::uuid _assetId;
		bool _bInitialized = false;

		CardSize _cardSize {};
		Control* _pCounterBG {};

		Control* _pLargeRoot {};
		Control* _pLargeFooter {};
		Control* _pLargeFooterFade {};
		Image* _pLargeBorder {};
		StaticText* _pLargeLabel {};
		Control* _pTagsRoot {};

		Control* _pSmallRoot {};
		Image* _pSmallBorder {};
		StaticText* _pSmallLabel {};

		Point _tagPosition {};
		int32_t _tagRows { 1 };

		fig::sdl::Surface _imageSurface {};
		fig::sdl::Texture _imageTexture {};
		fig::sdl::Surface _smallImageSurface {};
		fig::sdl::Texture _smallImageTexture {};
		fig::io::AsyncFuture _pendingCover {};

		bool _bHasError = false;
		Image* _pErrorIcon {};

		std::unique_ptr<SearchIndex> _searchIndex;
		std::set<fig::string> _tags {};
		bool _bEnableTags = true;

		struct PendingTag
		{
			fig::string tag;
			Color color;
		};
		fig::string _pendingLabel {};
		std::vector<PendingTag> _pendingTags {};

	};
}

#endif