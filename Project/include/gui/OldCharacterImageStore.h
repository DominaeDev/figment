#pragma once

#include "Types.h"
#include "gui/GUITypes.h"
#include <map>

namespace fig::gui
{
	enum class ImageType
	{
		Undefined = 0,
		Portrait_Square,
		Portrait_Large,
		Background,
	};

	class OldCharacterImageStore
	{
	public:
		static void Init(RendererPtr pRenderer);
		static void Release();
		static Texture* GetTexture(fig::string characterId, ImageType imageType);

		static bool LoadCharacterPortrait(RendererPtr pRenderer, fig::string characterId, fig::string filename);

	private:
		static bool LoadTexture(RendererPtr pRenderer, fig::string characterId, ImageType imageType, fig::string filename);

		struct CharacterImage
		{
			ImageType imageType = ImageType::Undefined;
			Texture* pTexture = nullptr;
		};

		using ImageList = std::vector<CharacterImage>;

		static std::map<fig::string, ImageList> _imagesByCharacter;
	};
}