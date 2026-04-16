#include <pch.h>
#include "gui/CoverCard.h"
#include "gui/TexturedBorder.h"
#include "gui/AppResources.h"
#include "gui/NineGridImage.h"
#include "gui/RoundedBorder.h"
#include "gui/GUIUtility.h"
#include "gui/MainFrame.h"
#include "util/StringUtility.h"

#include <cassert>

using namespace fig::util;

namespace fig::gui
{
	namespace Large
	{
		constexpr Coord Width = Constants::GUI::CardWidth;
		constexpr Coord Height = Constants::GUI::CardHeight;
		constexpr Coord Margin = 12;
		constexpr Coord FooterHeight = 80;
		constexpr float ZoomAmount = 18.0f;
	}

	namespace Small
	{
		constexpr Coord Width = Constants::GUI::HalfCardWidth;
		constexpr Coord Height = Constants::GUI::HalfCardHeight;
		constexpr Coord Margin = 10;
		constexpr Coord FooterHeight = 60;
		constexpr float ZoomAmount = 14.0f;
	}

	constexpr Coord kTagMargin = 10;
	constexpr Coord kTagSpacing = 6;
	constexpr Coord kTagInnerMargin = 8;
	constexpr Coord kTagMinWidth = 36;
	constexpr Coord kTagRowHeight = 32;
	constexpr Coord kTagY = 70;
	constexpr Coord kTagMaxRows = 2;
	
	constexpr uint8_t FadeAlpha = 0x60;
	constexpr float ZoomSmoothing = 8.0f;

	CoverCard::CoverCard(LayoutElement* pParent, const fig::uuid& assetId, CardSize cardSize) : CardImage(pParent, nullptr, AppResources::GetTexture(TextureType::MASK_CARD)),
		_assetId { assetId },
		_cardSize { cardSize }
	{
		_searchIndex = std::make_unique<SearchIndex>();

		_pHiddenBG = new TexturedBorder(this, TextureType::CARD_FILL, 8);
		_pHiddenBG->SetForegroundColor(Color { 0x9b896a, 0x30 });
		_pHiddenBorder = new TexturedBorder(_pHiddenBG, TextureType::CARD_BORDER, 16);
		_pHiddenBorder->SetForegroundColor(Color { 0, 0, 0, 0x80 });

		SetCardSize(cardSize);
		SetHidden(false);
	}

	void CoverCard::Initialize()
	{
		if (_bInitialized)
			return;

		_bInitialized = true;

		_pLargeRoot = new Area(this);
		_pLargeRoot->SetSize(Large::Width, Large::Height);
		_pSmallRoot = new Area(this);
		_pSmallRoot->SetSize(Small::Width, Small::Height);

		// Footer (large)
		_pLargeFooter = new Area(_pLargeRoot);
		_pLargeFooter->SetY(Large::Height - Large::FooterHeight);
		_pLargeFooter->SetSize(Large::Width, Large::FooterHeight);

		_pLargeFooterFade = new NineGridImage(_pLargeFooter, AppResources::GetTexture(TextureType::CARD_BOTTOM_FADE), { 16, 16, 64, 16 });
		_pLargeFooterFade->SetForegroundColor(Color { 0, 0, 0, FadeAlpha });
		_pLargeFooterFade->FillParent();

		// Label (large)
		_pLargeLabel = new StaticText(_pLargeFooter, "", FontFace::CardHeader, 28.0, false);
		_pLargeLabel->SetPosition(Large::Margin, Large::FooterHeight - Large::Margin - 68);
		_pLargeLabel->SetMaxSize(Large::Width - (Large::Margin * 2), -1);
		_pLargeLabel->SetSize(Large::Width - (Large::Margin * 2), 80);
		_pLargeLabel->SetForegroundColor(Colors::White);
		_pLargeLabel->SetBackgroundColor(Colors::Transparent);
		_pLargeLabel->EnableDropShadow(true);
		_pLargeLabel->EnableEllipsis(true);

		// Border (large)
		auto pSimpleBorder = new TexturedBorder(_pLargeRoot, AppResources::GetTexture(TextureType::CARD_BORDER), 16);
		pSimpleBorder->FillParent();
		pSimpleBorder->SetForegroundColor(Color { 0, 0, 0, FadeAlpha });

		// Styled border (large)
		_pLargeBorder = new Image(_pLargeRoot, nullptr);
		_pLargeBorder->SetPosition(-16, -16);
		_pLargeBorder->SetSize(Large::Width + 32, Large::Height + 32);
		_pLargeBorder->SetVisible(false);

		// Tags
		_tagPosition.x = kTagMargin;
		_tagPosition.y = kTagY;
		_pTagsRoot = new Area(_pLargeFooter);

		// Footer (small)
		auto pSmallFooter = new Area(_pSmallRoot);
		pSmallFooter->SetY(Small::Height - Small::FooterHeight);
		pSmallFooter->SetSize(Small::Width, Small::FooterHeight);

		auto pSmallFooterFade = new NineGridImage(pSmallFooter, AppResources::GetTexture(TextureType::CARD_BOTTOM_FADE_SMALL), { 16, 16, 40, 16 });
		pSmallFooterFade->SetForegroundColor(Color { 0, 0, 0, 0x80 });
		pSmallFooterFade->FillParent();

		// Label (small)
		_pSmallLabel = new StaticText(pSmallFooter, "", FontFace::CardHeader, 24.0, false);
		_pSmallLabel->SetPosition(Small::Margin, Small::FooterHeight - Small::Margin - 32);
		_pSmallLabel->SetMaxSize(Small::Width - (Small::Margin * 2), -1);
		_pSmallLabel->SetSize(Small::Width - (Small::Margin * 2), 40);
		_pSmallLabel->SetForegroundColor(Colors::White);
		_pSmallLabel->SetBackgroundColor(Colors::Transparent);
		_pSmallLabel->EnableDropShadow(true);
		_pSmallLabel->EnableEllipsis(true);

		// Border (small)
		auto pSmallSimpleBorder = new TexturedBorder(_pSmallRoot, AppResources::GetTexture(TextureType::CARD_BORDER), 16);
		pSmallSimpleBorder->FillParent();
		pSmallSimpleBorder->SetForegroundColor(Color { 0, 0, 0, 0x80 });

		// Styled border (small)
		_pSmallBorder = new Image(_pSmallRoot, nullptr);
		constexpr Coord borderOffset = static_cast<Coord>(16 * Constants::GUI::HalfScaleFactor);
		_pSmallBorder->SetPosition(-borderOffset, -borderOffset);
		_pSmallBorder->SetSize(Small::Width + borderOffset * 2, Small::Height + borderOffset * 2);
		_pSmallBorder->SetVisible(false);

		CreateChatCounter(0);
		SetCardSize(_cardSize);

		if constexpr (Enabled)
		{
			// Randomized border
			static std::mt19937_64 rng { std::default_random_engine{}() };

			constexpr std::array<CardBorderStyle, 16> borderWeights {
				 CardBorderStyle::None,
				 CardBorderStyle::None,
				 CardBorderStyle::None,
				 CardBorderStyle::None,
				 CardBorderStyle::None,
				 CardBorderStyle::None,
				 CardBorderStyle::None,
				 CardBorderStyle::None,
				 CardBorderStyle::None,
				 CardBorderStyle::None,
				 CardBorderStyle::Style01,
				 CardBorderStyle::Style02,
				 CardBorderStyle::Style03,
				 CardBorderStyle::Style04,
				 CardBorderStyle::Style05,
				 CardBorderStyle::Style06,
			};

			static std::uniform_int_distribution<size_t> dist(0, borderWeights.size() - 1);
			SetBorder(borderWeights[dist(rng)]);
		}

		CreatePendingTags();
		CreatePendingLabel();
		RefreshState();
	}

	void CoverCard::OnUpdate(float fElapsed)
	{
		if (!_bInitialized)
		{
			Initialize();
		}

		if (_bHasError && !_pErrorIcon)
		{
			// Create error icon
			float scale = _cardSize == CardSize::Full ? 1.0f : Constants::GUI::HalfScaleFactor;
			_pErrorIcon = new Image(this, AppResources::GetTexture(TextureType::ICON_ERROR));
			_pErrorIcon->SetSize(toI(_pErrorIcon->GetTextureSize().x * scale), toI(_pErrorIcon->GetTextureSize().y * scale));
			_pErrorIcon->SetForegroundColor(Color { 0xC0, 0xC0, 0xC0, });
			_pErrorIcon->Center();

			CardImage::SetTexture(AppResources::GetTexture(TextureType::CARD_BACKGROUND_EMPTY));
		}

		bool bHovered = (_bSelected or (is_inside(GetRect(), GetMousePos()))
			and !MainFrame::GetInstance().IsMenuShowing())
			and !_bHidden;

		if (bHovered != _bHovered)
		{
			_bHovered = bHovered;
			_fTargetZoom = bHovered ? 1.0f : 0.0f;
		}

		if (!flt_eq(_fHoverZoom, _fTargetZoom))
		{
			_fHoverZoom = std::clamp(_fHoverZoom + (_fTargetZoom - _fHoverZoom) * ZoomSmoothing * fElapsed, 0.0f, 1.0f);
			if (std::abs(_fTargetZoom - _fHoverZoom) < 0.01f)
				_fHoverZoom = _fTargetZoom;
			SetZoom(_fHoverZoom * (_cardSize == CardSize::Full ? Large::ZoomAmount : Small::ZoomAmount));
		}
	}

	void CoverCard::OnRender(Renderer* pRenderer)
	{
		if (_imageTexture.empty())
			PollFuture();

		CardImage::OnRender(pRenderer);
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
			if (_pLargeLabel->MeasureText(false).x <= _pLargeLabel->GetMaxSize().x)
			{
				_pLargeLabel->SetFont(Fonts::GetFont(FontFace::CardHeader, 28.0));
				_pLargeLabel->SetY(Large::FooterHeight - Large::Margin - 64 + (_bEnableTags ? 22 : 32));
			}
			else
			{
				_pLargeLabel->SetFont(Fonts::GetFont(FontFace::CardHeader, 24.0));
				_pLargeLabel->SetY(Large::FooterHeight - Large::Margin - 60 + (_bEnableTags ? 22 : 32));
			}
		}

		// Small label
		if (_pSmallLabel)
			_pSmallLabel->SetText(text);
	}

	void CoverCard::CreateChatCounter(uint32_t count)
	{
		if (_pCounterBG)
			return;

		_pCounterBG = new NineGridImage(this, AppResources::GetTexture(TextureType::CARD_TAG_BG), { 16, 16, 13, 13 });
		if (_cardSize == CardSize::Half)
			_pCounterBG->SetPosition(6, 6);
		else
			_pCounterBG->SetPosition(kTagMargin, kTagMargin);

		_pCounterBG->SetForegroundColor(Color { 0, 0, 0, 0xA0 });

		auto pCounterIcon = new Image(_pCounterBG, AppResources::GetTexture(TextureType::CARD_ICON_CHAT_COUNTER));
		pCounterIcon->SetPosition(6, 6);
		pCounterIcon->SetForegroundColor(Colors::White);
		pCounterIcon->SetBackgroundColor(Colors::Transparent);

		auto pLabel = new StaticText(_pCounterBG, std::format("{}", count), FontFace::Default, 14.0, true);
		pLabel->SetPosition(27, 3);
		pLabel->SetForegroundColor(Colors::White);
		pLabel->SetBackgroundColor(Colors::Transparent);

		auto [w, h] = pLabel->MeasureText();
		_pCounterBG->SetSize(std::max(w + 35, 32), 26);
	}

	static const Color& GetTagColor(const fig::string& tag)
	{
		static constexpr Color Black { 0x00, 0x00, 0x00, 0xA0 };
		return Black;

		static constexpr std::array<Color, 12> s_Colors {
			Color { 0xB3, 0x42, 0xC4, 0xB0 },
			Color { 0xC3, 0x30, 0x30, 0xB0 },
			Color { 0xF0, 0xAA, 0x46, 0xB0 },
			Color { 0x2C, 0xC6, 0xC4, 0xB0 },
			Color { 0x00, 0x95, 0x12, 0xB0 },
			Color { 0x90, 0x5D, 0x14, 0xB0 },
			Color { 0x43, 0xD0, 0xA3, 0xB0 },
			Color { 0x3C, 0x36, 0xB8, 0xB0 },
			Color { 0x66, 0xCC, 0x35, 0xB0 },
			Color { 0x86, 0x1E, 0x1E, 0xB0 },
			Color { 0xE6, 0x45, 0xA4, 0xB0 },
			Color { 0x31, 0x90, 0xC8, 0xB0 },
		};

		uint32_t n = 0uz;
		for (size_t i = 0; i < tag.size() && i < 16; ++i)
			n += static_cast<uint32_t>(tag[i]);
		n %= s_Colors.size();
		return s_Colors.at(n);
	}

	CoverCard::AddTagResult CoverCard::AddTag(const fig::string& tag, const Color& color)
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

		if (Large::Width < position.x + kTagInnerMargin * 2 + kTagMargin + kTagMinWidth)
		{
			if (_tagRows + 1 > kTagMaxRows)
				return AddTagResult::Stop;

			_tagPosition.x = kTagMargin;
			_tagPosition.y += kTagRowHeight;
			_tagRows++;

			_pLargeFooter->SetHeight(Large::FooterHeight + kTagRowHeight * _tagRows - 1);
			_pLargeFooter->SetY(Large::Height - _pLargeFooter->GetHeight());
			_pLargeFooterFade->FillParent();
			position = _tagPosition;
		}
		else if (_tags.empty())
		{
			_pLargeFooter->SetHeight(Large::FooterHeight + kTagRowHeight);
			_pLargeFooter->SetY(Large::Height - _pLargeFooter->GetHeight());
			_pLargeFooterFade->FillParent();
		}

		auto pTagBG = new NineGridImage(_pTagsRoot, AppResources::GetTexture(TextureType::CARD_TAG_BG), { 16, 16, 13, 13 });
		pTagBG->SetPosition(position);
		if (color.IsDefined())
			pTagBG->SetForegroundColor(color);
		else
			pTagBG->SetForegroundColor(GetTagColor(tag));

		auto pLabel = new StaticText(_pTagsRoot, tag, FontFace::Default, 14.0, true);
		pLabel->SetForegroundColor(Colors::White);
		pLabel->SetPosition(position.x + kTagInnerMargin, position.y + 3);
		pLabel->EnableWordWrap(false);
		pLabel->EnableEllipsis(true);
		pLabel->SetMaxSize(Large::Width - (position.x + kTagInnerMargin * 2 + kTagMargin), 0);

		auto [w, h] = pLabel->MeasureText();
		pTagBG->SetSize(w + kTagInnerMargin * 2, 26);

		_tagPosition.x += pTagBG->GetWidth() + kTagSpacing;
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

		TextureType textureType;
		switch (style)
		{
		case CardBorderStyle::Style01: textureType = TextureType::CARD_BORDER_STYLE_01; break;
		case CardBorderStyle::Style02: textureType = TextureType::CARD_BORDER_STYLE_02; break;
		case CardBorderStyle::Style03: textureType = TextureType::CARD_BORDER_STYLE_03; break;
		case CardBorderStyle::Style04: textureType = TextureType::CARD_BORDER_STYLE_04; break;
		case CardBorderStyle::Style05: textureType = TextureType::CARD_BORDER_STYLE_05; break;
		case CardBorderStyle::Style06: textureType = TextureType::CARD_BORDER_STYLE_06; break;
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
				_imageTexture.reset(pTexture);
				_imageSurface = std::move(full);
			}
			else
			{
				_imageTexture.clear();
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

	void CoverCard::SetPendingCoverImage(fig::io::AsyncFuture&& future)
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

		if (_pendingCover.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
		{
			if (auto result = _pendingCover.get(); result.has_value())
			{
				if (auto surface = std::get_if<fig::io::AsyncResult_Image>(&result.value()))
					SetCoverImages(std::move(*surface), {});
				else if (auto pair = std::get_if<fig::io::AsyncResult_CoverPair>(&result.value()))
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

	bool CoverCard::IsFilteredBy(const SearchQuery& query) const noexcept
	{
		return !_searchIndex->Match(query);
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
		if (_pHiddenBorder)
			_pHiddenBorder->FillParent();

		if (!_bInitialized)
			return;

		switch (cardSize)
		{
		default:
			_pLargeRoot->SetVisible(true);
			_pSmallRoot->SetVisible(false);
			if (_pCounterBG)
				_pCounterBG->SetPosition(kTagMargin, kTagMargin);
			SetSize(Large::Width, Large::Height);
			break;

		case CardSize::Half:
			_pLargeRoot->SetVisible(false);
			_pSmallRoot->SetVisible(true);
			if (_pCounterBG)
				_pCounterBG->SetPosition(6, 6);
			SetSize(Small::Width, Small::Height);
			break;
		}

		RefreshState();
	}

	void CoverCard::OnSize()
	{
		if (_pErrorIcon)
		{
			float scale = _cardSize == CardSize::Full ? 1.0f : Constants::GUI::HalfScaleFactor;
			_pErrorIcon->SetSize(toI(_pErrorIcon->GetTextureSize().x * scale), toI(_pErrorIcon->GetTextureSize().y * scale));
			_pErrorIcon->Center();
		}
		CardImage::OnSize();
	}

	void CoverCard::RefreshImage()
	{
		if (_cardSize == CardSize::Full)
		{
			if (!_imageTexture.empty())
				CardImage::SetTexture(_imageTexture.get());
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

	void CoverCard::EnableTags(bool bEnable)
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

			auto height = Large::FooterHeight + kTagRowHeight * _tagRows;
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

		_pLargeLabel->SetY(Large::FooterHeight - Large::Margin - 64 + (_bEnableTags ? 22 : 32));
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
			_pLargeRoot->SetVisible(_cardSize == CardSize::Full && !_bHidden);
		if (_pLargeBorder)
			_pLargeBorder->SetVisible(_pLargeBorder->HasTexture() && !_bHidden);
		if (_pSmallRoot)
			_pSmallRoot->SetVisible(_cardSize == CardSize::Half && !_bHidden);
		if (_pSmallBorder)
			_pSmallBorder->SetVisible(_pSmallBorder->HasTexture() && !_bHidden);
		if (_pCounterBG)
			_pCounterBG->SetVisible(!_bHidden);

		// Refresh image
		if (!_bHidden)
			RefreshImage();
		else
			CardImage::SetTexture(nullptr);
	}
}