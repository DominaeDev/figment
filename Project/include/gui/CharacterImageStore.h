#ifndef CHARACTER_IMAGE_STORE_H__
#define CHARACTER_IMAGE_STORE_H__

#pragma once

#include "Types.h"
#include "Graphics.h"

enum class ImageType
{
	Undefined = 0,
	Portrait_Square,
	Portrait_Large,
	Background,
};

class CharacterImageStore
{
public:
	static void Init(Renderer* pRenderer);
	static void Release();
	static Texture* GetTexture(string characterId, ImageType imageType);
	
	static bool LoadCharacterPortrait(string characterId, string filename);

private:
	static bool LoadTexture(Renderer* pRenderer, string characterId, ImageType imageType, string filename);

	struct CharacterImage
	{
		ImageType imageType = ImageType::Undefined;
		Texture* pTexture = nullptr;
	};

	using ImageList = std::vector<CharacterImage>;

	static std::map<string, ImageList> _imagesByCharacter;
};

#endif