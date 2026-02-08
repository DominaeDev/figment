#include <pch.h>
#include "gui/CharacterCard.h"
#include "model/AppState.h"
#include "model/UserManager.h"
#include "model/AssetManager.h"
#include "gui/Border.h"
#include "gui/TextureStore.h"
#include "gui/ImageStore.h"
#include "fs/Serialization.h"
#include <cassert>

using namespace fig::fs;

namespace fig::gui
{
	CharacterCard::CharacterCard(Control* pParent, const fig::uuid& characterId) : Image(pParent, nullptr),
		_characterId { characterId }
	{
		SetSize(Constants::GUI::CardWidth, Constants::GUI::CardHeight);

		if (ImageStore::LoadCoverImage(characterId))
		{
			auto coverId = ImageStore::GetCoverImageID(characterId);
			if (auto pTexture = ImageStore::GetTexture(GetSDLRenderer(), coverId))
				Image::SetTexture(pTexture);
		}

		auto pBorder = new Border(this, TextureStore::GetTexture(TextureType::CARD_BORDER), 16);
		pBorder->SetSize(GetSize());
		pBorder->SetForegroundColor(Color { 0, 0, 0, 0x80 });

		if (auto character = ApplicationState::GetUserManager().GetContent().GetCharacter(characterId))
		{
			auto pNameLabel = new StaticText(this, character.value().fullName, FontFace::CardHeader, 24.0, false);
			pNameLabel->SetMaxSize(GetWidth() - (12 * 2), -1);
			pNameLabel->SetSize(GetWidth() - (12 * 2), 80);
			pNameLabel->SetPosition(12, GetHeight() - 62);
			pNameLabel->SetForegroundColor(Colors::White);
			pNameLabel->SetBackgroundColor(Colors::Transparent);
			pNameLabel->EnableDropShadow(true);
			pNameLabel->EnableWordWrap(false);
			pNameLabel->EnableEllipsis(true);


			if (not character.value().subheader.empty())
			{
				auto pSubLabel = new StaticText(this, character.value().subheader, FontFace::CardHeader, 16.5, false);
				pSubLabel->SetMaxSize(GetWidth() - (12 * 2), -1);
				pSubLabel->SetSize(GetWidth() - (12 * 2), 80);
				pSubLabel->SetPosition(12, GetHeight() - 32);
				pSubLabel->SetForegroundColor(Colors::White);
				pSubLabel->SetBackgroundColor(Colors::Transparent);
				pSubLabel->EnableDropShadow(true);
				pSubLabel->EnableWordWrap(false);
				pSubLabel->EnableEllipsis(true);
			}
		}
	}

	void CharacterCard::OnRender(Renderer* pRenderer)
	{
		Image::OnRender(pRenderer);
		DrawBorder(pRenderer);
	}


}