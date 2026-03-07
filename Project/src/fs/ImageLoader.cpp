#include <pch.h>
#include "fs/ImageLoader.h"
#include "model/AppState.h"
#include "model/UserManager.h"
#include "model/AssetManager.h"
#include "Constants.h"
#include <cassert>

using namespace fig::io::data;
using namespace fig::gui;
using namespace fig::gui::util;

namespace fig::io
{
	void ImageLoader::Release()
	{
		_images.clear();
		_textures.clear();
		_coverImages.clear();
	}

	static bool CreateSurface(const Asset& cover, fig::sdl::Surface& out_surface)
	{
		// Create SDL surface
		int32_t width = cover.GetMeta<uint16_t>(MetaTag::ImageWidth).value_or(Constants::GUI::HomeScreen::CardWidth);
		int32_t height = cover.GetMeta<uint16_t>(MetaTag::ImageHeight).value_or(Constants::GUI::HomeScreen::CardHeight);
		int32_t depth = cover.GetMeta<uint8_t>(MetaTag::ImageFormatDepth).value_or(4);

		try
		{
			SurfacePtr pSurface = SDL_CreateSurface(width, height, depth == 3 ? SDL_PIXELFORMAT_RGB24 : SDL_PIXELFORMAT_RGBA8888);
			if (!pSurface)
				return false;

			if (SDL_LockSurface(pSurface))
			{
				assert(pSurface->pitch * pSurface->h == cover.data.size());
				std::memcpy(pSurface->pixels, cover.data.data(), cover.data.size());
				SDL_UnlockSurface(pSurface);

				out_surface.reset(pSurface);
				return true;
			}		
		}
		catch (...)
		{
		}
		return false;
	}

	bool ImageLoader::LoadCoverImage(const fig::uuid& characterAssetID)
	{
		auto& assetMngr = Global::GetUserManager().GetProfileAssets();

		if (auto findCover = assetMngr.FindAsset(characterAssetID, ImageType::CoverImage))
		{
			Asset& cover = findCover.value();
			if (_images.contains(cover.id))
				return true; // Already loaded

			if (assetMngr.LoadAsset(cover) == FileError::NoError)
			{
				fig::sdl::Surface surface;
				if (CreateSurface(cover, surface))
				{
					_images[cover.id] = std::move(surface);
					_coverImages[characterAssetID] = cover.id;
					return true;
				}
			}
		}

		// Create cover from portrait
		//if (auto findPortrait = assetMngr.FindAsset(characterAssetID, ImageType::LargePortrait))
		//{
		//	Asset& portraitAsset = findPortrait.value();
		//	if (assetMngr.LoadAsset(portraitAsset) == FileError::NoError)
		//	{
		//		if (auto portraitImage = LoadImageFromMemory(portraitAsset.data))
		//		{
		//			auto coverImage = ScaleSurface(portraitImage, Constants::GUI::HomeScreen::CardWidth, Constants::GUI::HomeScreen::CardHeight, ImageFit::Portrait);

		//			// Round corners
		//			MaskCorners(coverImage, CornerStyle::Card);

		//			// Save cover asset (bitmap)
		//			auto& coverAsset = assetMngr.CreateImageAsset(ImageType::CoverImage, coverImage, characterAssetID);
		//			coverAsset.SetMeta(MetaTag::Version, uint8_t { 1 });
		//			coverAsset.SetMeta(MetaTag::ReferenceToOriginal, portraitAsset.id);

		//			_images[coverAsset.id] = std::move(coverImage);
		//			_coverImages[characterAssetID] = coverAsset.id;
		//			return true;
		//		}
		//	}
		//}
		return false;
	}

	fig::uuid ImageLoader::GetCoverImageID(const fig::uuid& characterAssetID) noexcept
	{
		auto itFind = _coverImages.find(characterAssetID);
		if (itFind != _coverImages.cend())
			return itFind->second;
		return {};
	}

	TexturePtr ImageLoader::GetTexture(RendererPtr pRenderer, const fig::uuid& assetID)
	{
		auto itTexture = _textures.find(assetID);
		if (itTexture != _textures.end())
			return itTexture->second.get();

		auto itFind = _images.find(assetID);
		if (itFind != _images.end())
		{
			Surface* pSurface = itFind->second.get();
			auto pTexture = SDL_CreateTextureFromSurface(pRenderer, pSurface);
			if (!pTexture)
				return nullptr;

			fig::sdl::Texture texture;
			texture.reset(pTexture);
			_textures[assetID] = std::move(texture);
			return pTexture;
		}
		return nullptr; // Not found
	}
}