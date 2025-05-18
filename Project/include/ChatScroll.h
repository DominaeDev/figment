#pragma once

#include "Control.h"
#include "Types.h"

class ChatScroll : public Control
{
public:
	ChatScroll(Control* pParent);

	void AddMessage(string message, string speaker);

protected:
	void OnUpdate(float fDeltaTime) override {};
	void OnRender(SDL_Renderer* pRenderer) override {};
};