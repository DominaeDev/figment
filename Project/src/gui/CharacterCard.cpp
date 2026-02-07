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
	}

	void CharacterCard::OnRender(Renderer* pRenderer)
	{
		Image::OnRender(pRenderer);
		DrawBorder(pRenderer);
	}


}