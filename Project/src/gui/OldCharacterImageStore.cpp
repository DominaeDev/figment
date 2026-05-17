#include <pch.h>
#include "gui/OldCharacterImageStore.h"
#include <SDL3_image/SDL_image.h>

using namespace fig::gui;

std::map<fig::string, OldCharacterImageStore::ImageList> OldCharacterImageStore::_imagesByCharacter;

void OldCharacterImageStore::Init(RendererPtr pRenderer)
{
	LoadTexture(pRenderer, "Default", ImageType::Portrait_Square, "./resources/images/avatar_default.png");
	LoadTexture(pRenderer, "Female1", ImageType::Portrait_Square, "./resources/images/avatar_f1.png");
	LoadTexture(pRenderer, "Female2", ImageType::Portrait_Square, "./resources/images/avatar_f2.png");
	LoadTexture(pRenderer, "Female3", ImageType::Portrait_Square, "./resources/images/avatar_f3.png");
	LoadTexture(pRenderer, "Female4", ImageType::Portrait_Square, "./resources/images/avatar_f4.png");
	LoadTexture(pRenderer, "Male1", ImageType::Portrait_Square, "./resources/images/avatar_m1.png");
	LoadTexture(pRenderer, "Male2", ImageType::Portrait_Square, "./resources/images/avatar_m2.png");
	LoadTexture(pRenderer, "Male3", ImageType::Portrait_Square, "./resources/images/avatar_m3.png");
	LoadTexture(pRenderer, "Male4", ImageType::Portrait_Square, "./resources/images/avatar_m4.png");
}

void OldCharacterImageStore::Release()
{
	for (auto& character : _imagesByCharacter)
	{
		for (auto& img : character.second)
			SDL_DestroyTexture(img.pTexture);
	}
	_imagesByCharacter.clear();
}

bool OldCharacterImageStore::LoadCharacterPortrait(RendererPtr pRenderer, fig::string characterId, fig::string filename)
{
	return LoadTexture(pRenderer, characterId, ImageType::Portrait_Square, filename);
}

bool OldCharacterImageStore::LoadTexture(RendererPtr pRenderer, fig::string characterId, ImageType imageType, fig::string filename)
{
	if (imageType == ImageType::Undefined)
		return false;

	try
	{
		SurfacePtr pSurface = IMG_Load(filename.c_str());
		if (!pSurface)
			return false;

		auto pTexture = SDL_CreateTextureFromSurface(pRenderer, pSurface);
		SDL_DestroySurface(pSurface);

		if (!pTexture)
			return false;

		_imagesByCharacter[lcase(characterId)].push_back(CharacterImage {
			/*imageType*/ imageType,
			/*texture*/ pTexture,
			});
		return true;
	}
	catch (...)
	{
		return false;
	}
}

Texture* OldCharacterImageStore::GetTexture(fig::string characterId, ImageType imageType)
{
	if (characterId.empty())
		return nullptr;
	
	auto itCharacter = _imagesByCharacter.find(lcase(characterId));
	if (itCharacter != _imagesByCharacter.end())
	{
		auto& imageList = (*itCharacter).second;
		auto itImage = std::find_if(imageList.begin(), imageList.end(), [imageType](const CharacterImage& image) {
			return image.imageType == imageType;
		});
		if (itImage != imageList.end())
			return itImage->pTexture;
	}
	return nullptr;
}