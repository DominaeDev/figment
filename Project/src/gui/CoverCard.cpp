#include <pch.h>
#include "gui/CoverCard.h"
#include "gui/TexturedBorder.h"
#include "gui/AppResources.h"
#include "gui/NineGridImage.h"
#include "gui/RoundedBorder.h"
#include "util/StringUtility.h"

#include <cassert>

using namespace fig::util;

namespace fig::gui
{
	constexpr Coord kMargin = 12;

	constexpr Coord kTagMargin = 10;
	constexpr Coord kTagSpacing = 6;
	constexpr Coord kTagInnerMargin = 8;
	constexpr Coord kTagMinWidth = 36;
	constexpr Coord kTagRowHeight = 32;
	constexpr Coord kTagY = 70;
	constexpr Coord kTagMaxRows = 2;
	constexpr Coord kFooterHeight = 80;
	constexpr auto kLargeWidth = Constants::GUI::HomeScreen::CardWidth;
	constexpr auto kLargeHeight = Constants::GUI::HomeScreen::CardHeight;
	constexpr auto kSmallWidth = Constants::GUI::HomeScreen::CardWidth / 2;
	constexpr auto kSmallHeight = Constants::GUI::HomeScreen::CardHeight / 2;
	constexpr auto kSmallMargin = 8;

	CoverCard::CoverCard(LayoutElement* pParent, const fig::uuid& assetId, CardSize cardSize) : Image(pParent, nullptr),
		_assetId { assetId },
		_cardSize { cardSize }
	{
		_searchIndex = std::make_unique<SearchIndex>();
		
		if (cardSize == CardSize::Half)
			SetSize(kSmallWidth, kSmallHeight);
		else
			SetSize(kLargeWidth, kLargeHeight);
	}

	void CoverCard::Init()
	{
		if (_bInitialized)
			return;

		_bInitialized = true;

		_pLargeRoot = new Area(this);
		_pLargeRoot->SetSize(kLargeWidth, kLargeHeight);
		_pSmallRoot = new Area(this);
		_pSmallRoot->SetSize(kLargeWidth / 2, kLargeHeight / 2);

		// Footer (large)
		_pLargeFooter = new Area(_pLargeRoot);
		_pLargeFooter->SetY(kLargeHeight - kFooterHeight);
		_pLargeFooter->SetSize(kLargeWidth, kFooterHeight);

		_pLargeFooterFade = new NineGridImage(_pLargeFooter, AppResources::GetTexture(TextureType::CARD_BOTTOM_FADE), { 16, 16, 64, 16 });
		_pLargeFooterFade->SetForegroundColor(Color { 0, 0, 0, 0x40 });
		_pLargeFooterFade->FillParent();

		// Label (large)
		_pLargeLabel = new StaticText(_pLargeFooter, "", FontFace::CardHeader, 28.0, false);
		_pLargeLabel->SetPosition(kMargin, kFooterHeight - kMargin - 64);
		_pLargeLabel->SetMaxSize(kLargeWidth - (kMargin * 2), -1);
		_pLargeLabel->SetSize(kLargeWidth - (kMargin * 2), 80);
		_pLargeLabel->SetForegroundColor(Colors::White);
		_pLargeLabel->SetBackgroundColor(Colors::Transparent);
		_pLargeLabel->EnableDropShadow(true);
		_pLargeLabel->EnableEllipsis(true);

		// Border (large)
		auto pSimpleBorder = new TexturedBorder(_pLargeRoot, AppResources::GetTexture(TextureType::CARD_BORDER), 16);
		pSimpleBorder->FillParent();
		pSimpleBorder->SetForegroundColor(Color { 0, 0, 0, 0x40 });

		// Styled border (large)
		_pLargeBorder = new Image(_pLargeRoot, nullptr);
		_pLargeBorder->SetPosition(-16, -16);
		_pLargeBorder->SetSize(kLargeWidth + 32, kLargeHeight + 32);
		_pLargeBorder->SetVisible(false);

		// Tags
		_tagPosition.x = kTagMargin;
		_tagPosition.y = kTagY;
		_pTagsRoot = new Area(_pLargeFooter);

		// Footer (small)
		auto pSmallFooter = new Area(_pSmallRoot);
		pSmallFooter->SetSize(kSmallWidth, kSmallHeight);
		auto pSmallFooterFade = new NineGridImage(pSmallFooter, AppResources::GetTexture(TextureType::CARD_BOTTOM_FADE_SMALL), { 16, 16, 40, 16 });
		pSmallFooterFade->SetForegroundColor(Color { 0, 0, 0, 0x80 });
		pSmallFooterFade->SetWidth(kSmallWidth);
		pSmallFooterFade->SetY(kSmallHeight - pSmallFooterFade->GetHeight());

		_pSmallLabel = new StaticText(pSmallFooter, "", FontFace::CardHeader, 16.5, false);
		_pSmallLabel->SetPosition(kSmallMargin, kSmallHeight - kSmallMargin - 22);
		_pSmallLabel->SetMaxSize(kSmallWidth - (kSmallMargin * 2), -1);
		_pSmallLabel->SetSize(kSmallWidth - (kSmallMargin * 2), 40);
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
		_pSmallBorder->SetPosition(-6, -6);
		_pSmallBorder->SetSize(kSmallWidth + 12, kSmallHeight + 12);
		_pSmallBorder->SetVisible(false);

//		auto pSelectionBorder = new RoundedBorder(this, 8.0f, 6.0f, { 50, 200, 255 });
//		pSelectionBorder->SetPosition(-1, -1);
//		pSelectionBorder->SetSize(GetWidth() + 2, GetHeight() + 2);

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
	}

	void CoverCard::OnUpdate(float fElapsed)
	{
		if (!_bInitialized)
		{
			Init();
		}

		if (_bHasError && !_pErrorIcon)
		{
			// Create error icon
			int divide = _cardSize == CardSize::Default ? 1 : 2;
			_pErrorIcon = new Image(this, AppResources::GetTexture(TextureType::ICON_ERROR));
			_pErrorIcon->SetSize(_pErrorIcon->GetTextureSize().x / divide, _pErrorIcon->GetTextureSize().x / divide);
			_pErrorIcon->SetForegroundColor(Color { 0xC0, 0xC0, 0xC0, });
			_pErrorIcon->Center();

			Image::SetTexture(AppResources::GetTexture(TextureType::CARD_BACKGROUND_EMPTY));
		}
	}

	void CoverCard::OnRender(Renderer* pRenderer)
	{
		if (_imageTexture.empty())
			PollFuture();

		Image::OnRender(pRenderer);
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
				_pLargeLabel->SetY(kFooterHeight - kMargin - 64 + (_bEnableTags ? 22 : 32));
			}
			else
			{
				_pLargeLabel->SetFont(Fonts::GetFont(FontFace::CardHeader, 24.0));
				_pLargeLabel->SetY(kFooterHeight - kMargin - 60 + (_bEnableTags ? 22 : 32));
			}
		}

		// Small label
		if (_pSmallLabel)
			_pSmallLabel->SetText(text);
	}

	void CoverCard::CreateChatCounter(uint32_t count)
	{
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

		if (kLargeWidth < position.x + kTagInnerMargin * 2 + kTagMargin + kTagMinWidth)
		{
			if (_tagRows + 1 > kTagMaxRows)
				return AddTagResult::Stop;

			_tagPosition.x = kTagMargin;
			_tagPosition.y += kTagRowHeight;
			_tagRows++;

			_pLargeFooter->SetHeight(kFooterHeight + kTagRowHeight * _tagRows - 1);
			_pLargeFooter->SetY(kLargeHeight - _pLargeFooter->GetHeight());
			_pLargeFooterFade->FillParent();
			position = _tagPosition;
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
		pLabel->SetMaxSize(kLargeWidth - (position.x + kTagInnerMargin * 2 + kTagMargin), 0);

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

				RefreshImage();
				SetVisible(true);
			}
			else
			{
				_bHasError = true;
				SetVisible(true);
			}
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

	bool CoverCard::IsFilteredBy(const fig::string& search_string) const noexcept
	{
		return !_searchIndex->Match(search_string);
	}

	void CoverCard::SetCardSize(CardSize cardSize)
	{
		_cardSize = cardSize;

		if (cardSize == CardSize::Half)
			SetSize(kSmallWidth, kSmallHeight);
		else
			SetSize(kLargeWidth, kLargeHeight);

		if (!_bInitialized)
			return;

		switch (cardSize)
		{
		default:
			_pLargeRoot->SetVisible(true);
			_pSmallRoot->SetVisible(false);
			if (_pCounterBG)
				_pCounterBG->SetPosition(kTagMargin, kTagMargin);
			SetSize(kLargeWidth, kLargeHeight);
			break;

		case CardSize::Half:
			_pLargeRoot->SetVisible(false);
			_pSmallRoot->SetVisible(true);
			if (_pCounterBG)
				_pCounterBG->SetPosition(6, 6);
			SetSize(kSmallWidth, kSmallHeight);
			break;
		}

		RefreshImage();
	}

	void CoverCard::OnSize()
	{
		if (_pErrorIcon)
		{
			int divide = _cardSize == CardSize::Default ? 1 : 2;
			_pErrorIcon->SetSize(_pErrorIcon->GetTextureSize().x / divide, _pErrorIcon->GetTextureSize().x / divide);
			_pErrorIcon->Center();
		}
		Image::OnSize();
	}

	void CoverCard::RefreshImage()
	{
		if (_cardSize == CardSize::Default)
		{
			if (!_imageTexture.empty())
				Image::SetTexture(_imageTexture.get());
		}
		else
		{
			if (!_smallImageTexture.empty())
				Image::SetTexture(_smallImageTexture.get());
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

			auto height = kFooterHeight + kTagRowHeight * _tagRows;
			_pLargeFooter->SetHeight(height);
			_pLargeFooter->SetY(kLargeHeight - height);
			_pLargeFooterFade->FillParent();
		}
		else
		{
			_pLargeFooter->SetHeight(kFooterHeight);
			_pLargeFooter->SetY(kLargeHeight - kFooterHeight);
			_pLargeFooterFade->FillParent();
		}

		_pLargeLabel->SetY(kFooterHeight - kMargin - 64 + (_bEnableTags ? 22 : 32));
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
}