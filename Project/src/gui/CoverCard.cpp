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
	constexpr float Margin = 12.0f;

	constexpr float kTagMargin = 10;
	constexpr float kTagSpacing = 6;
	constexpr float kTagInnerMargin = 8;
	constexpr float kTagMinWidth = 36;
	constexpr float kTagRowHeight = 32;

	CoverCard::CoverCard(LayoutElement* pParent, const fig::uuid& assetId, CardSize cardSize) : Image(pParent, nullptr),
		_assetId { assetId },
		_cardSize { cardSize }
	{
		int divide = cardSize == CardSize::Default ? 1 : 2;
		SetSize(toF(Constants::GUI::HomeScreen::CardWidth / divide), toF(Constants::GUI::HomeScreen::CardHeight / divide));

		_pLargeFooter = new Area(this);
		_pLargeFooter->SetSize(GetSize());

		_pSimpleBorder = new TexturedBorder(this, AppResources::GetTexture(TextureType::CARD_BORDER), 16);
		_pSimpleBorder->SetSize(GetSize());
		_pSimpleBorder->SetForegroundColor(Color { 0, 0, 0, 0x80 });

		_pStyledBorder = new Image(this, nullptr);
		_pStyledBorder->SetPosition(-16, -16);
		_pStyledBorder->SetSize(GetWidth() + 32, GetHeight() + 32);
		_pStyledBorder->SetVisible(false);

		_pLargeFooterFade = new NineGridImage(_pLargeFooter, AppResources::GetTexture(TextureType::CARD_BOTTOM_FADE), { 16, 16, 64, 16 });
		_pLargeFooterFade->SetWidth(GetWidth());
		_pLargeFooterFade->SetY(GetHeight() - _pLargeFooterFade->GetHeight());
		_pLargeFooterFade->SetForegroundColor(Color { 0, 0, 0, 0x40 });

		_pSmallFooter = new Area(this);
		_pSmallFooter->SetSize(toF(GetWidth() / 2),toF(GetHeight() / 2));

		_tagPosition.x = kTagMargin;
		_tagPosition.y = GetHeight() - 35;

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

		Image::SetTexture(AppResources::GetTexture(TextureType::CARD_BACKGROUND_EMPTY));

//		auto pSelectionBorder = new RoundedBorder(this, 8.0f, 6.0f, { 50, 200, 255 });
//		pSelectionBorder->SetPosition(-1, -1);
//		pSelectionBorder->SetSize(GetWidth() + 2, GetHeight() + 2);

		SetCardSize(cardSize);
	}

	void CoverCard::OnUpdate(float fElapsed)
	{
		if (_bHasError && !_pErrorIcon)
		{
			// Create error icon
			int divide = _cardSize == CardSize::Default ? 1 : 2;
			_pErrorIcon = new Image(this, AppResources::GetTexture(TextureType::ICON_ERROR));
			_pErrorIcon->SetSize(toF(_pErrorIcon->GetTextureSize().x / divide), toF(_pErrorIcon->GetTextureSize().x / divide));
			_pErrorIcon->SetForegroundColor(Color { 0xC0, 0xC0, 0xC0, });
			_pErrorIcon->Center();
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
		// Name label (large)
		if (_pLabel)
		{
			_pLabel->SetText(text);
		}
		else
		{
			_pLabel = new StaticText(_pLargeFooter, text, FontFace::CardHeader, 28.0, false);
			_pLabel->SetPosition(Margin, GetHeight() - Margin - 64); // 58
			_pLabel->SetMaxSize(GetWidth() - (Margin * 2), -1);
			_pLabel->SetSize(GetWidth() - (Margin * 2), 80);
			_pLabel->SetForegroundColor(Colors::White);
			_pLabel->SetBackgroundColor(Colors::Transparent);
			_pLabel->EnableDropShadow(true);
			_pLabel->EnableWordWrap(false);
			_pLabel->EnableEllipsis(true);
		}

		if (_pLabel->MeasureText(false).x > _pLabel->GetMaxSize().x)
		{
			_pLabel->SetFont(Fonts::GetFont(FontFace::CardHeader, 24.0));
			_pLabel->SetY(_pLabel->GetY() + 4);
		}
	}

	void CoverCard::CreateChatCounter(uint32_t count)
	{
		_pCounterBG = new NineGridImage(this, AppResources::GetTexture(TextureType::CARD_TAG_BG), { 16, 16, 13, 13 });
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
		pLabel->EnableWordWrap(_pLargeFooter);

		auto [w, h] = pLabel->MeasureText();
		_pCounterBG->SetSize(toF(std::max(w + 35, 32)), 26);
	}

	static const Color& GetTagColor(const fig::string& tag)
	{
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

	CoverCard::AddTagResult CoverCard::AddTag(const fig::string& tag)
	{
		return AddTag(tag, {});
	}

	CoverCard::AddTagResult CoverCard::AddTag(const fig::string& tag, const Color& color)
	{
		if (tag.size() > 20)
			return AddTagResult::Reject; // Too long

		auto lower_tag = lcase(tag);
		if (_tags.contains(lower_tag))
			return AddTagResult::Reject; // Duplicate

		auto position = _tagPosition;

		if (position.x + kTagInnerMargin * 2 + kTagMargin + kTagMinWidth >= GetWidth())
		{
			if (++_tagRows > 2)
				return AddTagResult::Stop;

			_tagPosition.x = kTagMargin;
			_tagPosition.y += kTagRowHeight;

			_pLargeFooter->SetY(_pLargeFooter->GetY() - kTagRowHeight);
			_pLargeFooterFade->SetHeight(_pLargeFooterFade->GetHeight() + kTagRowHeight);
			position = _tagPosition;
		}

		auto pTagBG = new NineGridImage(_pLargeFooter, AppResources::GetTexture(TextureType::CARD_TAG_BG), { 16, 16, 13, 13 });
		pTagBG->SetPosition(position);
		if (color.IsDefined())
			pTagBG->SetForegroundColor(color);
		else
			pTagBG->SetForegroundColor(GetTagColor(tag));

		auto pLabel = new StaticText(_pLargeFooter, tag, FontFace::Default, 14.0, true);
		pLabel->SetPosition(position.x + kTagInnerMargin, position.y + 3);
		pLabel->SetForegroundColor(Colors::White);
		pLabel->SetBackgroundColor(Colors::Transparent);
		pLabel->EnableWordWrap(false);
		pLabel->EnableEllipsis(true);
		pLabel->SetMaxSize(GetWidth() - (position.x + kTagInnerMargin * 2 + kTagMargin), 0);

		auto [w, h] = pLabel->MeasureText();
		pTagBG->SetSize(toF(w + kTagInnerMargin * 2), 26);

		_tagPosition.x += pTagBG->GetWidth() + kTagSpacing;
		_tags.insert(lower_tag);
		return AddTagResult::Ok;
	}

	void CoverCard::SetBorder(CardBorderStyle style)
	{
		if (style == None)
		{
			_pStyledBorder->SetTexture(nullptr);
			_pStyledBorder->SetVisible(false);
			_pSimpleBorder->SetVisible(true);
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
			_pStyledBorder->SetTexture(pTexture);
			_pStyledBorder->SetSize(toF(pTexture->w), toF(pTexture->h));
			_pStyledBorder->SetVisible((bool)pTexture);
			_pSimpleBorder->SetVisible(false);
		}
		else
		{
			_pStyledBorder->SetTexture(nullptr);
			_pStyledBorder->SetVisible(false);
			_pSimpleBorder->SetVisible(true);
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
			}
			else
			{
				_bHasError = true;
			}
		}
	}

	static fig::wstring normalize_search_term(const fig::string& text)
	{
		auto result = lcase(from_utf8(text));
		return strip_diacritics(std::move(result));
	}

	static fig::wstring normalize_search_term(const fig::wstring& text)
	{
		auto result = lcase(text);
		return strip_diacritics(std::move(result));
	}

	static std::unordered_set<fig::wstring> filter_search(const fig::string& text)
	{
		static constexpr auto is_delimiter = [](char c) {
			return std::ispunct(static_cast<unsigned char>(c))
				or std::isspace(static_cast<unsigned char>(c));
		};

		auto chunks = text
			| std::views::chunk_by([&](char a, char b) { return !is_delimiter(a) && !is_delimiter(b); })
			| std::views::transform([](auto&& range) { 
				auto s = fig::string(std::ranges::begin(range), std::ranges::end(range));
				return normalize_search_term(s);
			})
			| std::ranges::to<std::unordered_set>();

		return chunks
			| std::views::filter([&](auto& s) { return not empty_or_whitespace(s); })
			| std::ranges::to<std::unordered_set>();
	}

	void CoverCard::AddSearchText(const fig::string& text) noexcept
	{
		if (not text.empty())
			_searchWords.emplace_back(normalize_search_term(text));
	}

	void CoverCard::AddSearchText(const std::span<fig::string> texts) noexcept
	{
		for (auto& t : texts)
			AddSearchText(t);
	}

	void CoverCard::AddSearchText(const std::span<fig::wstring> texts) noexcept
	{
		for (auto& t : texts)
			AddSearchText(t);
	}

	void CoverCard::AddSearchText(const fig::wstring& text) noexcept
	{
		if (not text.empty())
			_searchWords.emplace_back(normalize_search_term(text));
	}

	bool CoverCard::IsFilteredBy(const fig::string& search_string) const noexcept
	{
		auto search_words = filter_search(search_string);

		for (auto& word : search_words)
		{
			bool bFound = false;
			for (auto& s : _searchWords)
				bFound |= fig::util::find_in(word, s, false, true);
			if (!bFound)
				return true;
		}
		return false;
	}

	void CoverCard::SetCardSize(CardSize cardSize)
	{
		_cardSize = cardSize;

		// Create small label
		if (cardSize == CardSize::Half and not (bool)_pSmallLabel and (bool)_pLabel)
		{
			int32_t kWidth = toI(GetWidth()) / 2;
			int32_t kHeight = toI(GetHeight()) / 2;
			constexpr int32_t kMargin = toI(Margin / 2);

			_pSmallLabel = new StaticText(_pSmallFooter, _pLabel->GetText(), FontFace::CardHeader, 16.5, false);
			_pSmallLabel->SetPosition(toF(kMargin), toF(kHeight - kMargin - 22));
			_pSmallLabel->SetMaxSize(toF(kWidth - (kMargin * 2)), -1);
			_pSmallLabel->SetSize(toF(kWidth - (kMargin * 2)), 40);
			_pSmallLabel->SetForegroundColor(Colors::White);
			_pSmallLabel->SetBackgroundColor(Colors::Transparent);
			_pSmallLabel->EnableDropShadow(true);
			_pSmallLabel->EnableWordWrap(false);
			_pSmallLabel->EnableEllipsis(true);

		}

		int divide;
		switch (cardSize)
		{
		default:
			if (_pLargeFooter)
				_pLargeFooter->SetVisible(true);
			if (_pSmallFooter)
				_pSmallFooter->SetVisible(false);
			if (_pCounterBG)
				_pCounterBG->SetPosition(kTagMargin, kTagMargin);
			divide = 1;
			break;
		case CardSize::Half:
			if (_pLargeFooter)
				_pLargeFooter->SetVisible(false);
			if (_pSmallFooter)
				_pSmallFooter->SetVisible(true);
			if (_pCounterBG)
				_pCounterBG->SetPosition(6, 6);
			divide = 2;
			break;
		}

		SetSize(toF(Constants::GUI::HomeScreen::CardWidth / divide), toF(Constants::GUI::HomeScreen::CardHeight / divide));
		_pSimpleBorder->SetSize(GetSize());
		float borderMargin = cardSize == CardSize::Default ? 16.0f : 6.0f;
		_pStyledBorder->SetPosition(-borderMargin, -borderMargin);
		_pStyledBorder->SetSize(GetWidth() + borderMargin * 2, GetHeight() + borderMargin * 2);

		RefreshImage();
	}

	void CoverCard::OnSize()
	{
		if (_pErrorIcon)
		{
			int divide = _cardSize == CardSize::Default ? 1 : 2;
			_pErrorIcon->SetSize(toF(_pErrorIcon->GetTextureSize().x / divide), toF(_pErrorIcon->GetTextureSize().x / divide));
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
			else
				Image::SetTexture(AppResources::GetTexture(TextureType::CARD_BACKGROUND_EMPTY));
		}
		else
		{
			if (!_smallImageTexture.empty())
				Image::SetTexture(_smallImageTexture.get());
			else
				Image::SetTexture(AppResources::GetTexture(TextureType::CARD_BACKGROUND_EMPTY));
		}

	}
}