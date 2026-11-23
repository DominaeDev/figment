#include <pch.h>
#include "gui/PortraitImage.h"
#include "gui/AppResources.h"
#include "io/AssetManager.h"
#include "io/ContentManager.h"

using namespace fig::io;

namespace fig::gui
{
	PortraitImage::PortraitImage(ControlPtr pParent) : NonOwningImageWithMask(pParent)
	{
		_pDefaultImage = AppResources::GetTexture(Resource::PROFILE_DEFAULT_IMAGE);
		SetTexture(_pDefaultImage, false);
		SetMask(AppResources::GetTexture(Resource::ROUNDED_BACKGROUND_6PX), 8);
	}

	void PortraitImage::SetCharacter(const fig::uuid& characterId)
	{
		if (auto try_asset = Global::GetUserContent().GetAssets().FindAssetOfType(make_asset_type(AssetType::Image, ImageAssetType::SmallPortrait), characterId))
		{
			if (auto portrait = Global::GetUserContent().Get<fig::sdl::Surface>((*try_asset).id))
			{
				if (auto pTexture = SDL_CreateTextureFromSurface(GetSDLRenderer(), (*portrait).get()))
				{
					SetTexture(pTexture, false);
					_portraitTexture.reset(pTexture);
					return;
				}
			}
		}

		// Failed
		SetDefault();
	}

	void PortraitImage::SetDefault()
	{
		_portraitTexture.reset();
		SetTexture(_pDefaultImage, false);
	}
}