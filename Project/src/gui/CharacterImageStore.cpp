#include "gui/CharacterImageStore.h"
#include <SDL3_image/SDL_image.h>

std::map<string, CharacterImageStore::ImageList> CharacterImageStore::_imagesByCharacter;

void CharacterImageStore::Init(Renderer* pRenderer)
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

void CharacterImageStore::Release()
{
	for (auto& character : _imagesByCharacter)
	{
		for (auto& img : character.second)
			SDL_DestroyTexture(img.pTexture);
	}
	_imagesByCharacter.clear();
}

bool CharacterImageStore::LoadTexture(Renderer* pRenderer, string characterId, ImageType imageType, const char* filename)
{
	if (imageType == ImageType::Undefined)
		return false;

	auto pSurface = IMG_Load(filename);
	if (!pSurface)
		return false;

	auto pTexture = SDL_CreateTextureFromSurface(pRenderer, pSurface);
	SDL_DestroySurface(pSurface);

	if (!pTexture)
		return false;

	_imagesByCharacter[characterId].push_back(CharacterImage {
		/*imageType*/ imageType,
		/*texture*/ pTexture,
	});
	return true;
}

Texture* CharacterImageStore::GetTexture(string characterId, ImageType imageType)
{
	auto itCharacter = _imagesByCharacter.find(characterId);
	if (itCharacter != std::end(_imagesByCharacter))
	{
		auto& imageList = (*itCharacter).second;
		auto itImage = std::find_if(std::begin(imageList), std::end(imageList), [imageType](const CharacterImage& image) {
			return image.imageType == imageType;
		});
		if (itImage != std::end(imageList))
			return itImage->pTexture;
	}
	return nullptr;
}