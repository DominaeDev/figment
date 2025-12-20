#include "gui/CharacterImageStore.h"
#include "model/AppState.h"
#include "util/StringUtility.h"
#include <SDL3_image/SDL_image.h>

using namespace fig::string_util;

std::map<fig::string, CharacterImageStore::ImageList> CharacterImageStore::_imagesByCharacter;

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

bool CharacterImageStore::LoadCharacterPortrait(fig::string characterId, fig::string filename)
{
	return LoadTexture(ApplicationState::GetRenderer(), characterId, ImageType::Portrait_Square, filename);
}

bool CharacterImageStore::LoadTexture(Renderer* pRenderer, fig::string characterId, ImageType imageType, fig::string filename)
{
	if (imageType == ImageType::Undefined)
		return false;

	try
	{
		auto pSurface = IMG_Load(filename.c_str());
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

Texture* CharacterImageStore::GetTexture(fig::string characterId, ImageType imageType)
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