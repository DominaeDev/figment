#pragma once

#include "gui/GUITypes.h"
#include <map>

namespace fig::gui
{
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

		CARD_BORDER,
		CARD_BOTTOM_FADE,
		CARD_TAG_BG,
		CARD_ICON_CHAT_COUNTER,
		CARD_ICON_FAVORITE_OFF,
		CARD_ICON_FAVORITE_ON,
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
}