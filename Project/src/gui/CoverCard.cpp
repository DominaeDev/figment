#include <pch.h>
#include "gui/CoverCard.h"
#include "gui/TexturedBorder.h"
#include "gui/AppResources.h"
#include "gui/NineGridImage.h"
#include "gui/RoundedBorder.h"
#include "gui/GUIUtility.h"
#include "gui/MainFrame.h"

#include <cassert>

using namespace fig::io;
using namespace fig::data;

namespace fig::gui
{
	namespace Large = Constants::GUI::Cards::Full;
	namespace Small = Constants::GUI::Cards::Half;

	constexpr uint8_t FadeAlpha = 0x60;

	CoverCard::CoverCard(ControlPtr pParent, const fig::uuid& assetId, CardSize cardSize) : CardImage(pParent, nullptr, AppResources::GetTexture(Resource::MASK_CARD)),
		_assetId { assetId },
		_cardSize { cardSize }
	{
		_searchIndex = std::make_unique<SearchIndex>();

		_pHiddenBG = CreateControl<TexturedBorder>(Resource::CARD_FILL, 8);
		_pHiddenBG->SetForegroundColor(fig::color { 0x9b896a, 0x30 });

		SetCardSize(cardSize);
		SetHidden(false);
	}

	void CoverCard::Initialize()
	{
		if (_bInitialized)
			return;

		_bInitialized = true;

		_pLargeRoot = CreateControl<Area>();
		_pLargeRoot->SetSize(Large::Width, Large::Height);
		_pSmallRoot = CreateControl<Area>();
		_pSmallRoot->SetSize(Small::Width, Small::Height);

		// Footer (large)
		_pLargeFooter = _pLargeRoot->CreateControl<Area>();
		_pLargeFooter->SetY(Large::Height - Large::FooterHeight);
		_pLargeFooter->SetSize(Large::Width, Large::FooterHeight);

		_pLargeFooterFade = _pLargeFooter->CreateControl<NineGridImage>(AppResources::GetTexture(Resource::CARD_BOTTOM_FADE), fig::corners { 16, 16, 64, 16 });
		_pLargeFooterFade->SetForegroundColor(fig::color { 0, 0, 0, FadeAlpha });
		_pLargeFooterFade->FillParent();
		_pLargeFooterFade->SetVisible(false);

		// Label (large)
		_pLargeLabel = _pLargeFooter->CreateControl<StaticText>("", FontFace::CardHeader, 28.0, false);
		_pLargeLabel->SetPosition(Large::InnerMargin, Large::FooterHeight - Large::InnerMargin - 68);
		_pLargeLabel->SetSize(Large::Width - (Large::InnerMargin * 2), 80);
		_pLargeLabel->SetForegroundColor(Color::White);
		_pLargeLabel->SetBackgroundColor(Color::Transparent);
		_pLargeLabel->EnableDropShadow(true);
		_pLargeLabel->EnableEllipsis(true);

		// Border (large)
		auto pSimpleBorder = _pLargeRoot->CreateControl<TexturedBorder>(AppResources::GetTexture(Resource::CARD_BORDER), 16);
		pSimpleBorder->FillParent();
		pSimpleBorder->SetForegroundColor(fig::color { 0, 0, 0, FadeAlpha });

		// Styled border (large)
		_pLargeBorder = _pLargeRoot->CreateControl<Image>(nullptr);
		_pLargeBorder->SetPosition(-Large::BorderOffset, -Large::BorderOffset);
		_pLargeBorder->SetSize(Large::Width + Large::BorderOffset * 2, Large::Height + Large::BorderOffset * 2);
		_pLargeBorder->SetVisible(false);

		// Star (large)
		_pLargeStar = _pLargeRoot->CreateControl<Image>(Resource::CARD_ICON_STAR);
		_pLargeStar->SetPosition(Large::Width - _pLargeStar->GetWidth() - 8, 8);
		_pLargeStar->SetVisible(false);

		// Tags
		_tagPosition.x = Large::Tags::Margin;
		_tagPosition.y = Large::Tags::Top;
		_pTagsRoot = _pLargeFooter->CreateControl<Area>();

		// Footer (small)
		auto pSmallFooter = _pSmallRoot->CreateControl<Area>();
		pSmallFooter->SetY(Small::Height - Small::FooterHeight);
		pSmallFooter->SetSize(Small::Width, Small::FooterHeight);

		_pSmallFooterFade = pSmallFooter->CreateControl<NineGridImage>(AppResources::GetTexture(Resource::CARD_BOTTOM_FADE_SMALL), fig::corners { 16, 16, 40, 16 });
		_pSmallFooterFade->SetForegroundColor(fig::color { 0, 0, 0, 0x80 });
		_pSmallFooterFade->FillParent();
		_pSmallFooterFade->SetVisible(false);

		// Label (small)
		_pSmallLabel = pSmallFooter->CreateControl<StaticText>("", FontFace::CardHeader, 24.0, false);
		_pSmallLabel->SetPosition(Small::InnerMargin, Small::FooterHeight - Small::InnerMargin - Small::TextY);
		_pSmallLabel->SetSize(Small::Width - (Small::InnerMargin * 2), 40);
		_pSmallLabel->SetForegroundColor(Color::White);
		_pSmallLabel->SetBackgroundColor(Color::Transparent);
		_pSmallLabel->EnableDropShadow(true);
		_pSmallLabel->EnableEllipsis(true);

		// Border (small)
		auto pSmallSimpleBorder = _pSmallRoot->CreateControl<TexturedBorder>(AppResources::GetTexture(Resource::CARD_BORDER), 16);
		pSmallSimpleBorder->FillParent();
		pSmallSimpleBorder->SetForegroundColor(fig::color { 0, 0, 0, 0x80 });

		// Styled border (small)
		_pSmallBorder = _pSmallRoot->CreateControl<Image>(nullptr);
		_pSmallBorder->SetPosition(-Small::BorderOffset, -Small::BorderOffset);
		_pSmallBorder->SetSize(Small::Width + Small::BorderOffset * 2, Small::Height + Small::BorderOffset * 2);
		_pSmallBorder->SetVisible(false);

		// Star (small)
		_pSmallStar = _pSmallRoot->CreateControl<Image>(Resource::CARD_ICON_STAR_SMALL);
		_pSmallStar->SetPosition(Small::Width - _pSmallStar->GetWidth() - 6, 6);
		_pSmallStar->SetVisible(false);

		SetChatCount(_metaData.chatCount);
		SetCardSize(_cardSize);

		SetBorder(_userSettings.borderStyle);
		ShowStar(_userSettings.HasFlag(ContentUserSettings::Flag::Favorite));

		CreatePendingTags();
		CreatePendingLabel();
		RefreshState();
	}

	void CoverCard::OnUpdate(float fElapsed)
	{
		if (_largeImageTexture.empty())
			PollFuture();

		if (!_bInitialized)
		{
			Initialize();
		}

		if (_bHasError && !_pErrorIcon)
		{
			// Create error icon
			float scale = _cardSize == CardSize::Full ? 1.0f : 0.75f;
			_pErrorIcon = CreateControl<Image>(AppResources::GetTexture(Resource::ICON_ERROR));
			_pErrorIcon->SetSize(toI(_pErrorIcon->GetTextureSize().x * scale), toI(_pErrorIcon->GetTextureSize().y * scale));
			_pErrorIcon->SetForegroundColor(fig::color { 0xC0, 0xC0, 0xC0, });
			_pErrorIcon->Center();

			CardImage::SetTexture(AppResources::GetTexture(Resource::CARD_BACKGROUND_EMPTY));
		}

		bool bHovered = (_bSelected or is_inside(GetRect(), GetMousePos())
			and !MainFrame::GetInstance().IsMenuShowing())
			and !_bHidden;

		if (bHovered != _bHovered)
		{
			_bHovered = bHovered;
			_fTargetZoom = bHovered ? 1.0f : 0.0f;
		}

		if (!flt_eq(_fHoverZoom, _fTargetZoom))
		{
			_fHoverZoom = std::clamp(_fHoverZoom + (_fTargetZoom - _fHoverZoom) * Constants::GUI::Cards::ZoomSmoothing * fElapsed, 0.0f, 1.0f);
			if (std::abs(_fTargetZoom - _fHoverZoom) < 0.065f)
				_fHoverZoom = _fTargetZoom;
			SetZoom(_fHoverZoom);
		}
	}

	void CoverCard::SetLabel(const fig::string& text) noexcept
	{
		if (!_bInitialized)
		{
			_pendingLabel = text;
			return;
		}

		// Large label
		if (_pLargeLabel)
		{
			_pLargeLabel->SetText(text);

			// Adjust font size
			if (_pLargeLabel->MeasureText(false).x <= _pLargeLabel->GetMaxWidth())
			{
				_pLargeLabel->SetFont(Fonts::GetFont(FontFace::CardHeader, 28.0));
				_pLargeLabel->SetY(Large::FooterHeight - Large::InnerMargin - 64 + (_bEnableTags ? 22 : 32));
			}
			else
			{
				_pLargeLabel->SetFont(Fonts::GetFont(FontFace::CardHeader, 24.0));
				_pLargeLabel->SetY(Large::FooterHeight - Large::InnerMargin - 60 + (_bEnableTags ? 22 : 32));
			}
		}

		// Small label
		if (_pSmallLabel)
			_pSmallLabel->SetText(text);
	}

	void CoverCard::SetChatCount(uint32_t count)
	{
		if (!_pCounterBG)
		{
			_pCounterBG = CreateControl<NineGridImage>(AppResources::GetTexture(Resource::CARD_TAG_BG), fig::corners { 16, 16, 13, 13 });
			if (_cardSize == CardSize::Half)
				_pCounterBG->SetPosition(6, 6);
			else
				_pCounterBG->SetPosition(Large::Tags::Margin, Large::Tags::Margin);

			_pCounterBG->SetForegroundColor(0xA0_rgba);

			auto pCounterIcon = _pCounterBG->CreateControl<Image>(AppResources::GetTexture(Resource::CARD_ICON_CHAT_COUNTER));
			pCounterIcon->SetPosition(6, 6);
			pCounterIcon->SetForegroundColor(Color::White);
			pCounterIcon->SetBackgroundColor(Color::Transparent);

			_pChatCount = _pCounterBG->CreateControl<StaticText>("", FontFace::Default, 14.0, true);
			_pChatCount->SetPosition(27, 3);
			_pChatCount->SetForegroundColor(Color::White);
			_pChatCount->SetBackgroundColor(Color::Transparent);
		}

		if (_pChatCount)
		{
			_pChatCount->SetText(std::format("{}", count));
			auto [w, h] = _pChatCount->MeasureText();
			_pCounterBG->SetSize(std::max(w + 35, 32), 26);
		}

	}

	void CoverCard::ShowNew(bool bShow)
	{
		if (_pNewIndicator)
		{
			_pNewIndicator->SetVisible(bShow and !_bHidden);
			return;
		}
		else if (!bShow)
			return;

		_pNewIndicator = CreateControl<NineGridImage>(AppResources::GetTexture(Resource::CARD_TAG_BG), fig::corners { 16, 16, 13, 13 });
		if (_cardSize == CardSize::Half)
			_pNewIndicator->SetPosition(6, 6);
		else
			_pNewIndicator->SetPosition(Large::Tags::Margin, Large::Tags::Margin);

		_pNewIndicator->SetForegroundColor(0x1065b4E0_rgba);

		auto pLabel = _pNewIndicator->CreateControl<StaticText>(fig::string { fig::strings::UI::New }, FontFace::Default, 14.0, true);
		pLabel->SetPosition(6, 3);
		pLabel->SetForegroundColor(Color::White);
		pLabel->SetBackgroundColor(Color::Transparent);

		auto [w, h] = pLabel->MeasureText();
		_pNewIndicator->SetSize(w + 12, 26);
		_pNewIndicator->SetVisible(!_bHidden);
	}

	CoverCard::AddTagResult CoverCard::AddTag(const fig::string& tag, const fig::color& color)
	{
		if (!_bInitialized or !_bEnableTags)
		{
			_pendingTags.push_back(PendingTag { tag, color });
			return CoverCard::AddTagResult::Reject;
		}

		if (tag.size() > 20)
			return AddTagResult::Reject; // Too long

		auto lower_tag = lcase(tag);
		if (_tags.contains(lower_tag))
			return AddTagResult::Reject; // Duplicate

		auto position = _tagPosition;

		if (Large::Width < position.x + Large::Tags::InnerMargin * 2 + Large::Tags::Margin + Large::Tags::MinWidth)
		{
			if (_tagRows + 1 > Large::Tags::MaxRows)
				return AddTagResult::Stop;

			_tagPosition.x = Large::Tags::Margin;
			_tagPosition.y += Large::Tags::RowHeight;
			_tagRows++;

			_pLargeFooter->SetHeight(Large::FooterHeight + Large::Tags::RowHeight * _tagRows - 1);
			_pLargeFooter->SetY(Large::Height - _pLargeFooter->GetHeight());
			_pLargeFooterFade->FillParent();
			position = _tagPosition;
		}
		else if (_tags.empty())
		{
			_pLargeFooter->SetHeight(Large::FooterHeight + Large::Tags::RowHeight);
			_pLargeFooter->SetY(Large::Height - _pLargeFooter->GetHeight());
			_pLargeFooterFade->FillParent();
		}

		auto pTagBG = _pTagsRoot->CreateControl<NineGridImage>(AppResources::GetTexture(Resource::CARD_TAG_BG), fig::corners { 16, 16, 13, 13 });
		pTagBG->SetPosition(position);
		pTagBG->SetForegroundColor(Color::Black.WithAlpha(0.7f));

		auto pLabel = _pTagsRoot->CreateControl<StaticText>(tag, FontFace::Default, 14.0, true);
		pLabel->SetPosition(position.x + Large::Tags::InnerMargin, position.y + 3);
		pLabel->EnableWordWrap(false);
		pLabel->EnableEllipsis(true);
		pLabel->SetMaxWidth(Large::Width - (position.x + Large::Tags::InnerMargin * 2 + Large::Tags::Margin));
		if (color.IsDefined())
			pLabel->SetForegroundColor(color);
		else
			pLabel->SetForegroundColor(Color::White);

		auto [w, h] = pLabel->MeasureText();
		pTagBG->SetSize(w + Large::Tags::InnerMargin * 2, 26);

		_tagPosition.x += pTagBG->GetWidth() + Large::Tags::Spacing;
		_tags.insert(lower_tag);
		return AddTagResult::Ok;
	}

	void CoverCard::SetBorder(CardBorderStyle style)
	{
		if (style == None)
		{
			_pLargeBorder->SetTexture(nullptr);
			_pLargeBorder->SetVisible(false);
			_pSmallBorder->SetTexture(nullptr);
			_pSmallBorder->SetVisible(false);
			return;
		}

		Resource textureType;
		switch (style)
		{
		case CardBorderStyle::Style01: textureType = Resource::CARD_BORDER_STYLE_01; break;
		case CardBorderStyle::Style02: textureType = Resource::CARD_BORDER_STYLE_02; break;
		case CardBorderStyle::Style03: textureType = Resource::CARD_BORDER_STYLE_03; break;
		case CardBorderStyle::Style04: textureType = Resource::CARD_BORDER_STYLE_04; break;
		case CardBorderStyle::Style05: textureType = Resource::CARD_BORDER_STYLE_05; break;
		case CardBorderStyle::Style06: textureType = Resource::CARD_BORDER_STYLE_06; break;
		default:
			return;
		};

		if (auto pTexture = AppResources::GetTexture(textureType))
		{
			_pLargeBorder->SetTexture(pTexture);
			_pLargeBorder->SetVisible((bool)pTexture);
			_pSmallBorder->SetTexture(pTexture);
			_pSmallBorder->SetVisible((bool)pTexture);
		}
		else
		{
			_pLargeBorder->SetTexture(nullptr);
			_pLargeBorder->SetVisible(false);
			_pSmallBorder->SetTexture(nullptr);
			_pSmallBorder->SetVisible(false);
		}
	}

	void CoverCard::SetCoverImages(fig::sdl::Surface&& full, fig::sdl::Surface&& half)
	{
		auto pRenderer = GetSDLRenderer();

		if (!full.empty())
		{
			if (auto pTexture = SDL_CreateTextureFromSurface(pRenderer, full.get()))
			{
				_largeImageTexture.reset(pTexture);
				_imageSurface = std::move(full);
			}
			else
			{
				_largeImageTexture.clear();
				_imageSurface.clear();
			}
		}

		if (!half.empty())
		{
			if (auto pTexture = SDL_CreateTextureFromSurface(pRenderer, half.get()))
			{
				_smallImageTexture.reset(pTexture);
				_smallImageSurface = std::move(half);
			}
			else
			{
				_smallImageTexture.clear();
				_smallImageSurface.clear();
			}
		}
	}

	void CoverCard::SetPendingCoverImage(AsyncFuture&& future)
	{
		if (not future.valid())
			return;

		_pendingCover = std::move(future);
		PollFuture();
	}

	void CoverCard::PollFuture()
	{
		if (not _pendingCover.valid())
			return;

		if (_pendingCover.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
		{
			if (auto result = _pendingCover.get(); result.has_value())
			{
				if (auto surface = std::get_if<AsyncResult_Image>(&result.value()))
					SetCoverImages(std::move(*surface), {});
				else if (auto pair = std::get_if<AsyncResult_CoverPair>(&result.value()))
					SetCoverImages(std::move(pair->first), std::move(pair->second));
			}
			else
			{
				_bHasError = true;
			}
			RefreshState();
		}
	}

	void CoverCard::AddSearchTerms(const fig::string& text) noexcept
	{
		_searchIndex->AddTerm(text);
	}

	void CoverCard::AddSearchTerms(std::span<const fig::string> texts) noexcept
	{
		_searchIndex->AddTerms(texts);
	}

	bool CoverCard::MatchesFlags(FilterFlags filter) const noexcept
	{
		if (filter.IsSet(FilterFlag::Hidden))
			return _userSettings.HasFlag(ContentUserSettings::Flag::Hidden);
		if (_userSettings.HasFlag(ContentUserSettings::Flag::Hidden))
			return false;

		auto [name, gender, _] = _metaData.gender.Get();
		switch (gender)
		{
		case ConventionalGender::Male:
			if (not filter.IsSet(FilterFlag::GenderMale))
				return false;
			break;
		case ConventionalGender::Female:
			if (not filter.IsSet(FilterFlag::GenderFemale))
				return false;
			break;
		default:
			if (not filter.IsSet(FilterFlag::GenderOther))
				return false;
			break;
		}

		if (filter.IsSet(FilterFlag::New) and not _metaData.IsNew())
			return false;
		if (filter.IsSet(FilterFlag::Chats) and _metaData.chatCount == 0)
			return false;
		if (filter.IsSet(FilterFlag::Starred) and not _userSettings.HasFlag(ContentUserSettings::Flag::Favorite))
			return false;
		if (not filter.IsSet(FilterFlag::SourceCreated) and not _userSettings.HasFlag(ContentUserSettings::Flag::Imported))
			return false;
		if (not filter.IsSet(FilterFlag::SourceImported) and _userSettings.HasFlag(ContentUserSettings::Flag::Imported))
			return false;

		return true;
	}

	void CoverCard::SetDelegate(OnCardUpdatedDelegate onUpdated)
	{
		_fnOnUpdated = onUpdated;
	}

	void CoverCard::NotifyMetaUpdated()
	{
		if (_fnOnUpdated)
			_fnOnUpdated(*this);
	}

	bool CoverCard::MatchesSearch(const SearchQuery& query) const noexcept
	{
		return query.empty() or _searchIndex->Match(query);
	}

	void CoverCard::SetCardSize(CardSize cardSize)
	{
		_cardSize = cardSize;

		if (cardSize == CardSize::Half)
			SetSize(Small::Width, Small::Height);
		else
			SetSize(Large::Width, Large::Height);

		if (_pHiddenBG)
			_pHiddenBG->FillParent();

		if (!_bInitialized)
			return;

		switch (cardSize)
		{
		default:
			_pLargeRoot->SetVisible(true);
			_pSmallRoot->SetVisible(false);
			if (_pCounterBG)
				_pCounterBG->SetPosition(Large::Tags::Margin, Large::Tags::Margin);
			if (_pNewIndicator)
				_pNewIndicator->SetPosition(Large::Tags::Margin + (_pCounterBG ? _pCounterBG->GetWidth() + 4 : 0), Large::Tags::Margin);
			_fZoomExpand = toF(Constants::GUI::Cards::Full::ZoomPixels);
			break;

		case CardSize::Half:
			_pLargeRoot->SetVisible(false);
			_pSmallRoot->SetVisible(true);
			if (_pCounterBG)
				_pCounterBG->SetPosition(6, 6);
			if (_pNewIndicator)
				_pNewIndicator->SetPosition(6 + (_pCounterBG ? _pCounterBG->GetWidth() + 4 : 0), 6);
			_fZoomExpand = toF(Constants::GUI::Cards::Half::ZoomPixels);
			break;
		}

		RefreshState();
	}

	void CoverCard::OnSize()
	{
		if (_pErrorIcon)
		{
			float scale = _cardSize == CardSize::Full ? 1.0f : 0.75f;
			_pErrorIcon->SetSize(toI(_pErrorIcon->GetTextureSize().x * scale), toI(_pErrorIcon->GetTextureSize().y * scale));
			_pErrorIcon->Center();
		}
		CardImage::OnSize();
	}

	void CoverCard::RefreshImage()
	{
		if (_cardSize == CardSize::Full)
		{
			if (!_largeImageTexture.empty())
				CardImage::SetTexture(_largeImageTexture.get());
		}
		else
		{
			if (!_smallImageTexture.empty())
				CardImage::SetTexture(_smallImageTexture.get());
		}
	}

	void CoverCard::SetIndex(const SearchIndex& index) noexcept
	{
		_searchIndex = std::make_unique<SearchIndex>(index);
	}

	void CoverCard::ShowTags(bool bEnable)
	{
		if (_bEnableTags == bEnable)
			return;
		_bEnableTags = bEnable;

		if (!_bInitialized)
			return;

		_pTagsRoot->SetVisible(bEnable);
		if (_bEnableTags)
		{
			CreatePendingTags();

			auto height = Large::FooterHeight + Large::Tags::RowHeight * _tagRows;
			_pLargeFooter->SetHeight(height);
			_pLargeFooter->SetY(Large::Height - height);
			_pLargeFooterFade->FillParent();
		}
		else
		{
			_pLargeFooter->SetHeight(Large::FooterHeight);
			_pLargeFooter->SetY(Large::Height - Large::FooterHeight);
			_pLargeFooterFade->FillParent();
		}

		_pLargeLabel->SetY(Large::FooterHeight - Large::InnerMargin - 64 + (_bEnableTags ? 22 : 32));
	}

	void CoverCard::ShowStar(bool bShow)
	{
		if (_pLargeStar)
			_pLargeStar->SetVisible(bShow and !_bHidden);
		if (_pSmallStar)
			_pSmallStar->SetVisible(bShow and !_bHidden);
	}

	void CoverCard::CreatePendingTags()
	{
		if (not _pendingTags.empty() and _bInitialized and _bEnableTags)
		{
			for (auto& tag : _pendingTags)
			{
				if (AddTag(tag.tag, tag.color) == AddTagResult::Stop)
					break;
			}
			_pendingTags.clear();
		}
	}

	void CoverCard::CreatePendingLabel()
	{
		if (not _pendingLabel.empty() and _bInitialized)
		{
			SetLabel(_pendingLabel);
			_pendingLabel.clear();
		}
	}

	void CoverCard::SetHidden(bool bHidden)
	{
		_bHidden = bHidden;
		RefreshState();
	}

	void CoverCard::RefreshState()
	{
		if (_cardSize == CardSize::Half)
			SetSize(Small::Width, Small::Height);
		else
			SetSize(Large::Width, Large::Height);

		// Hidden?
		if (_pHiddenBG)
			_pHiddenBG->SetVisible(_bHidden);
		if (_pLargeRoot)
			_pLargeRoot->SetVisible(_cardSize == CardSize::Full and !_bHidden);
		if (_pLargeBorder)
			_pLargeBorder->SetVisible(_pLargeBorder->HasTexture() and !_bHidden);
		if (_pLargeFooterFade)
			_pLargeFooterFade->SetVisible(!_largeImageTexture.empty());
		if (_pLargeStar)
			_pLargeStar->SetVisible(_userSettings.HasFlag(ContentUserSettings::Flag::Favorite) and !_bHidden);

		if (_pSmallRoot)
			_pSmallRoot->SetVisible(_cardSize == CardSize::Half and !_bHidden);
		if (_pSmallBorder)
			_pSmallBorder->SetVisible(_pSmallBorder->HasTexture() and !_bHidden);
		if (_pSmallFooterFade)
			_pSmallFooterFade->SetVisible(!_smallImageTexture.empty());
		if (_pSmallStar)
			_pSmallStar->SetVisible(_userSettings.HasFlag(ContentUserSettings::Flag::Favorite) and !_bHidden);

		if (_pCounterBG)
			_pCounterBG->SetVisible(!_bHidden);
		if (_pNewIndicator)
		{
			_pNewIndicator->SetVisible(_metaData.IsNew() and !_bHidden);

			if (_cardSize == CardSize::Full)
				_pNewIndicator->SetPosition(Large::Tags::Margin + (_pCounterBG ? _pCounterBG->GetWidth() + 4 : 0), Large::Tags::Margin);
			else
				_pNewIndicator->SetPosition(6 + (_pCounterBG ? _pCounterBG->GetWidth() + 4 : 0), 6);
		}

		// Refresh image
		if (!_bHidden)
			RefreshImage();
		else
			CardImage::SetTexture(nullptr);
	}

	void CoverCard::ResetHoverZoom()
	{
		_bHovered = false;
		_fHoverZoom = 0.0f;
		_fTargetZoom = 0.0f;
		SetZoom(0.0f);
	}

	void CoverCard::SetMetaData(const fig::io::ContentMetaData& metaData) noexcept
	{
		_metaData = metaData;
		ShowNew(metaData.IsNew());
		SetChatCount(metaData.chatCount);
	}

	void CoverCard::SetUserSettings(const ContentUserSettings& userSettings) noexcept
	{
		_userSettings = userSettings;
		ShowStar(userSettings.HasFlag(ContentUserSettings::Flag::Favorite));
	}
}