#pragma once

#include <map>
#include "Graphics.h"

enum class TextureType
{
	BLANK,
	BORDER,
	
	TEXTBOX_BG,
	TEXTBOX_BORDER,

	SPEECH_BUBBLE_LEFT_BG,
	SPEECH_BUBBLE_LEFT_BORDER,
	SPEECH_BUBBLE_CENTER_BG,
	SPEECH_BUBBLE_CENTER_BORDER,
	SPEECH_BUBBLE_RIGHT_BG,
	SPEECH_BUBBLE_RIGHT_BORDER,
};

class TextureStore
{
public:
	static void Init(Renderer* pRenderer);
	static void Release();
	static Texture* GetTexture(TextureType id);

private:
	static bool LoadTexture(Renderer* pRenderer, TextureType textureId, const char* filename);

	static std::map<TextureType, Texture*> _textures;
};
