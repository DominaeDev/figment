#include "gui/Text.h"

TTF_TextEngine* Text::_pEngine = nullptr;

TTF_TextEngine* Text::InitEngine(SDL_Renderer* pRenderer)
{
	_pEngine = TTF_CreateRendererTextEngine(pRenderer);
	return _pEngine;
}
