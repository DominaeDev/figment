export module Text;

import <SDL3_ttf/SDL_ttf.h>;
import Fonts;
import Graphics;

export
{
	struct TTF_TextEngine;
	struct TTF_Font;

	class Text
	{
	public:
		static TTF_TextEngine* InitEngine(Renderer* pRenderer);
		static TTF_TextEngine* GetEngine() { return _pEngine; }

	private:
		static TTF_TextEngine* _pEngine;
	};
}

TTF_TextEngine* Text::_pEngine = nullptr;

TTF_TextEngine* Text::InitEngine(Renderer* pRenderer)
{
	_pEngine = TTF_CreateRendererTextEngine(pRenderer);
	return _pEngine;
}
