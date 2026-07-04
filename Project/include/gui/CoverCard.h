#pragma once

#include "CardImage.h"
#include "io/AssetManager.h"
#include "data/CardMetaData.h"
#include "user/UserSettings.h"
#include "util/SearchIndex.h"

namespace fig::gui
{
	enum class CardSize
	{
		Full,
		Half,
	};

	class TexturedBorder;
	class NineGridImage;
	class CoverCard;

	using OnCardUpdatedDelegate = std::function<void(CoverCard&)>;

	class CoverCard : public CardImage
	{
	public:
		CoverCard(LayoutElement* pParent, const fig::uuid& assetId, CardSize cardSize = CardSize::Full);
		void Initialize();

		void SetBorder(fig::data::CardBorderStyle style);
		void SetCardSize(CardSize cardSize);
		void SetPendingCoverImage(fig::io::AsyncFuture&& future);
		void ShowTags(bool bEnable);
		void ShowStar(bool bShow);
		void SetHidden(bool bHidden);
		inline bool IsHidden() const noexcept { return _bHidden; }
		
		bool MatchesFlags(FilterFlags filter) const noexcept;
		bool MatchesSearch(const SearchQuery& query) const noexcept;

		const fig::uuid& GetAssetID() const { return _assetId; }
		inline const fig::data::CardMetaData& GetMetaData() const noexcept { return _metaData; };

		void SetDelegate(OnCardUpdatedDelegate onUpdated);
		void ResetHoverZoom();

	protected:
		void SetCoverImages(fig::sdl::Surface&& fullCover, fig::sdl::Surface&& halfCover);
		void RefreshState();

		void SetLabel(const fig::string& text) noexcept;
		void SetChatCount(uint32_t count);
		void ShowNew(bool bShow);

		enum class AddTagResult { Ok, Reject, Stop };
		AddTagResult AddTag(const fig::string& tag, const Color& color = {});

		void OnUpdate(float fElapsed) override;
		void OnSize() override;
		
		void SetIndex(const SearchIndex& index) noexcept;
		void AddSearchTerms(const fig::string& text) noexcept;
		void AddSearchTerms(std::span<const fig::string> texts) noexcept;

		inline void SetMetaData(const fig::data::CardMetaData& metaData) noexcept { _metaData = metaData; };
		void NotifyMetaUpdated();

	private:
		void RefreshImage();
		void PollFuture();
		void CreatePendingTags();
		void CreatePendingLabel();
		
	protected:
		fig::uuid _assetId;
		bool _bSelected = false;
		bool _bInitialized = false;
		bool _bHidden = false;
		bool _bHasError = false;
		OnCardUpdatedDelegate _fnOnUpdated {};
		fig::data::CardMetaData _metaData {};
		bool _bHovered = false;
		float _fHoverZoom = 0.0f;
		float _fTargetZoom = 0.0f;

	private:
		TexturedBorder* _pHiddenBG {};

		CardSize _cardSize {};
		Control* _pCounterBG {};
		Control* _pNewIndicator {};

		Control* _pLargeRoot {};
		Control* _pLargeFooter {};
		Control* _pLargeFooterFade {};
		Image* _pLargeBorder {};
		StaticText* _pLargeLabel {};
		Image* _pLargeStar {};
		Control* _pTagsRoot {};

		Control* _pSmallRoot {};
		Control* _pSmallFooterFade {};
		Image* _pSmallBorder {};
		StaticText* _pSmallLabel {};
		Image* _pSmallStar {};

		Point _tagPosition {};
		int32_t _tagRows { 1 };

		fig::sdl::Surface _imageSurface {};
		fig::sdl::Texture _largeImageTexture {};
		fig::sdl::Surface _smallImageSurface {};
		fig::sdl::Texture _smallImageTexture {};
		fig::io::AsyncFuture _pendingCover {};

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
