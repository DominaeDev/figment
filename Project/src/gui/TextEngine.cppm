export module GUI.Text.TextEngine;

import <SDL3_ttf/SDL_ttf.h>;
export import GUI.GraphicTypes;

export
{
	struct TTF_TextEngine;
	struct TTF_Font;

	class TextEngine
	{
	public:
		static TTF_TextEngine* InitEngine(Renderer* pRenderer)
		{
			_pEngine = TTF_CreateRendererTextEngine(pRenderer);
			return _pEngine;
		}

		static TTF_TextEngine* GetEngine() { return _pEngine; }

	private:
		static TTF_TextEngine* _pEngine;
	};
}

TTF_TextEngine* TextEngine::_pEngine = nullptr;