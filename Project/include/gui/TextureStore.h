#pragma once

#include <map>

struct SDL_Renderer;
struct SDL_Texture;

enum class Texture
{
	BORDER = 0,
	BG_9GRID,
	BORDER_9GRID,
};

class TextureStore
{
public:
	static void Init(SDL_Renderer* pRenderer);
	static void Release();
	static SDL_Texture* GetTexture(Texture id);

private:
	static bool LoadTexture(SDL_Renderer* pRenderer, Texture textureId, const char* filename);

	static std::map<Texture, SDL_Texture*> _textures;
};
