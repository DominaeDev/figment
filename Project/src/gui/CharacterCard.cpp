#include <pch.h>
#include "gui/CharacterCard.h"
#include "model/AppState.h"
#include "model/UserManager.h"
#include "model/AssetManager.h"
#include "gui/Border.h"
#include "gui/TextureStore.h"
#include "fs/Serialization.h"
#include <cassert>

using namespace fig::fs;

namespace fig::gui
{
	CharacterCard::CharacterCard(Control* pParent) : Image(pParent, nullptr)
	{
		SetSize(Constants::GUI::CardWidth, Constants::GUI::CardHeight);

		auto pBorder = new Border(this, TextureStore::GetTexture(TextureType::CARD_BORDER), 16);
		pBorder->SetSize(GetSize());
		pBorder->SetForegroundColor(Color { 0, 0, 0, 0x80 });
	}

	CharacterCard::CharacterCard(Control* pParent, const fig::uuid& characterId) : Image(pParent, nullptr),
		_characterId { characterId }
	{
		SetSize(Constants::GUI::CardWidth, Constants::GUI::CardHeight);

		LoadCharacterPortrait();

		auto pBorder = new Border(this, TextureStore::GetTexture(TextureType::CARD_BORDER), 16);
		pBorder->SetSize(GetSize());
		pBorder->SetForegroundColor(Color { 0, 0, 0, 0x80 });
	}

	bool CharacterCard::LoadCharacterPortrait()
	{
		_surface.reset();
		_texture.reset();

		if (_characterId.empty())
			return false; // Invalid id

		auto& assets = ApplicationState::GetUserManager().GetProfileAssets();
		const auto& characterId = _characterId;

		// Find character
		auto findPortrait = assets.GetAssets()
			| std::views::filter([&characterId](const auto& a) { return a.parent_id == characterId and a.IsOfImageType(ImageType::CoverImage); })
			| std::views::take(1);
		
		if (findPortrait.empty())
			return false; // Not found

		auto& portrait = findPortrait.front();
		if (auto result = assets.LoadAsset(portrait.id))
		{
			// Create texture
			auto& asset = result.value().get();
			int32_t width = asset.GetMeta<int32_t>(MetaTag::ImageWidth).value_or(Constants::GUI::CardWidth);
			int32_t height = asset.GetMeta<int32_t>(MetaTag::ImageHeight).value_or(Constants::GUI::CardHeight);
			int32_t depth = asset.GetMeta<int32_t>(MetaTag::ImageFormatDepth).value_or(4);

			try
			{
				auto* pRenderer = GetSDLRenderer();

				SurfacePtr pSurface = SDL_CreateSurface(width, height, depth == 3 ? SDL_PIXELFORMAT_RGB24 : SDL_PIXELFORMAT_RGBA8888);
				if (!pSurface)
					return false;
				_surface.reset(pSurface);

				if (SDL_LockSurface(pSurface))
				{
					assert(pSurface->pitch * pSurface->h == asset.data.size());
					std::memcpy(pSurface->pixels, asset.data.data(), asset.data.size());
					SDL_UnlockSurface(pSurface);
				}
					
				auto pTexture = SDL_CreateTextureFromSurface(pRenderer, pSurface);
				if (!pTexture)
					return false;
				_texture.reset(pTexture);

				Image::SetTexture(pTexture);
				return true;
			}
			catch (...)
			{
			}
		}
	
		return false;
	}

	void CharacterCard::OnRender(Renderer* pRenderer)
	{
		Image::OnRender(pRenderer);
		DrawBorder(pRenderer);
	}


}