#pragma once

#include "Types.h"
#include "Graphics.h"
#include <map>

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
	static Texture* GetTexture(fig::string characterId, ImageType imageType);
	
	static bool LoadCharacterPortrait(fig::string characterId, fig::string filename);

private:
	static bool LoadTexture(Renderer* pRenderer, fig::string characterId, ImageType imageType, fig::string filename);

	struct CharacterImage
	{
		ImageType imageType = ImageType::Undefined;
		Texture* pTexture = nullptr;
	};

	using ImageList = std::vector<CharacterImage>;

	static std::map<fig::string, ImageList> _imagesByCharacter;
};
