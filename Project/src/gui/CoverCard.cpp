#include <pch.h>
#include "gui/CoverCard.h"
#include "gui/Border.h"
#include "gui/ImageStore.h"
#include "gui/TextureStore.h"
#include "gui/NineGridImage.h"
#include <cassert>

namespace fig::gui
{
	constexpr float Margin = 12.0f;

	CoverCard::CoverCard(Control* pParent, const fig::uuid& assetId) : Image(pParent, nullptr),
		_assetId { assetId }
	{
		SetSize(Constants::GUI::CardWidth, Constants::GUI::CardHeight);

		if (ImageStore::LoadCoverImage(assetId))
		{
			auto coverId = ImageStore::GetCoverImageID(assetId);
			if (auto pTexture = ImageStore::GetTexture(GetSDLRenderer(), coverId))
				Image::SetTexture(pTexture);
		}

		auto pBottomFade = new NineGridImage(this, TextureStore::GetTexture(TextureType::CARD_BOTTOM_FADE), { 16, 16, 64, 16 });
		pBottomFade->SetWidth(GetWidth());
		pBottomFade->SetY(GetHeight() - pBottomFade->GetHeight());
		pBottomFade->SetForegroundColor(Color { 0, 0, 0, 0x40 });

		auto pBorder = new Border(this, TextureStore::GetTexture(TextureType::CARD_BORDER), 16);
		pBorder->SetSize(GetSize());
		pBorder->SetForegroundColor(Color { 0, 0, 0, 0x80 });

		auto pFavoriteOff = new Image(this, TextureStore::GetTexture(TextureType::CARD_ICON_FAVORITE_OFF));
		pFavoriteOff->SetPosition(GetWidth() - 42, 8);
		pFavoriteOff->SetForegroundColor(Color { 0x60, 0x60, 0x60, 0x60 });
	}

	void CoverCard::OnRender(Renderer* pRenderer)
	{
		Image::OnRender(pRenderer);
	}

	void CoverCard::SetLabel(const fig::string& text) noexcept
	{
		if (_pLabel)
		{
			RemoveChild(_pLabel);
			delete _pLabel;
		}

		_pLabel = new StaticText(this, text, FontFace::CardHeader, 24.0, false);
		_pLabel->SetMaxSize(GetWidth() - (Margin * 2), -1);
		_pLabel->SetSize(GetWidth() - (Margin * 2), 80);
		_pLabel->SetPosition(Margin, GetHeight() - Margin - 58);
		_pLabel->SetForegroundColor(Colors::White);
		_pLabel->SetBackgroundColor(Colors::Transparent);
		_pLabel->EnableDropShadow(true);
		_pLabel->EnableWordWrap(false);
		_pLabel->EnableEllipsis(true);
	}

	void CoverCard::SetSublabel(const fig::string& text) noexcept
	{
		if (_pSublabel)
		{
			RemoveChild(_pSublabel);
			delete _pSublabel;
		}

		_pSublabel = new StaticText(this, text, FontFace::CardSubheader, 16.5, false);
		_pSublabel->SetMaxSize(GetWidth() - (Margin * 2), -1);
		_pSublabel->SetSize(GetWidth() - (Margin * 2), 80);
		_pSublabel->SetPosition(Margin, GetHeight() - Margin - 28);
		_pSublabel->SetForegroundColor(Colors::White);
		_pSublabel->SetBackgroundColor(Colors::Transparent);
		_pSublabel->EnableDropShadow(true);
		_pSublabel->EnableWordWrap(false);
		_pSublabel->EnableEllipsis(true);
	}

	void CoverCard::CreateChatCounter(uint32_t count)
	{
		auto position = Pointf { 10, GetHeight() - 35 };
		auto pCounterBG = new NineGridImage(this, TextureStore::GetTexture(TextureType::CARD_TAG_BG), { 16, 16, 13, 13 });
		pCounterBG->SetPosition(position);
		pCounterBG->SetForegroundColor(Color { 0, 0, 0, 96 });

		auto pCounterIcon = new Image(this, TextureStore::GetTexture(TextureType::CARD_ICON_CHAT_COUNTER));
		pCounterIcon->SetPosition(position.x + 6, position.y + 6);
		pCounterIcon->SetForegroundColor(Colors::White);
		pCounterIcon->SetBackgroundColor(Colors::Transparent);

		auto pLabel = new StaticText(this, std::format("{}", count), FontFace::Default, 14.0, true);
		pLabel->SetPosition(position.x + 27, position.y + 3);
		pLabel->SetForegroundColor(Colors::White);
		pLabel->SetBackgroundColor(Colors::Transparent);
		pLabel->EnableWordWrap(false);

		auto [w, h] = pLabel->MeasureText();
		pCounterBG->SetSize(toF(std::max(w + 35, 32)), 26);

	}
}