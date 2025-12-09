export module CharacterImageStore;

import Common;
import GUI.GraphicTypes;
import AppState;

export
{
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
}

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

bool CharacterImageStore::LoadCharacterPortrait(string characterId, string filename)
{
	return LoadTexture(Application::GetRenderer(), characterId, ImageType::Portrait_Square, filename);
}

bool CharacterImageStore::LoadTexture(Renderer* pRenderer, string characterId, ImageType imageType, string filename)
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

		_imagesByCharacter[string_util::lcase(characterId)].push_back(CharacterImage {
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

Texture* CharacterImageStore::GetTexture(string characterId, ImageType imageType)
{
	if (characterId.empty())
		return nullptr;

	auto itCharacter = _imagesByCharacter.find(string_util::lcase(characterId));
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