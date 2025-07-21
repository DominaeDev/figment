#include "gui/Text.h"

TTF_TextEngine* Text::_pEngine = nullptr;

TTF_TextEngine* Text::InitEngine(Renderer* pRenderer)
{
	_pEngine = TTF_CreateRendererTextEngine(pRenderer);
	return _pEngine;
}
