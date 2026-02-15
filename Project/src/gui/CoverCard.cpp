#include <pch.h>
#include "gui/CoverCard.h"
#include "gui/Border.h"
#include "gui/ImageStore.h"
#include "gui/TextureStore.h"
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

		auto pBorder = new Border(this, TextureStore::GetTexture(TextureType::CARD_BORDER), 16);
		pBorder->SetSize(GetSize());
		pBorder->SetForegroundColor(Color { 0, 0, 0, 0x80 });
	}

	void CoverCard::OnRender(Renderer* pRenderer)
	{
		Image::OnRender(pRenderer);
		DrawBorder(pRenderer);
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
		_pLabel->SetPosition(Margin, GetHeight() - Margin - 52);
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
		_pSublabel->SetPosition(Margin, GetHeight() - Margin - 22);
		_pSublabel->SetForegroundColor(Colors::White);
		_pSublabel->SetBackgroundColor(Colors::Transparent);
		_pSublabel->EnableDropShadow(true);
		_pSublabel->EnableWordWrap(false);
		_pSublabel->EnableEllipsis(true);
	}
}