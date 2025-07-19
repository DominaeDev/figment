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
	static void Init(SDL_Renderer* pRenderer);
	static void Release();
	static SDL_Texture* GetTexture(string characterId, ImageType imageType);

private:
	static bool LoadTexture(SDL_Renderer* pRenderer, string characterId, ImageType imageType, const char* filename);

	struct CharacterImage
	{
		ImageType imageType = ImageType::Undefined;
		SDL_Texture* pTexture = nullptr;
	};

	using ImageList = std::vector<CharacterImage>;

	static std::map<string, ImageList> _imagesByCharacter;
};
