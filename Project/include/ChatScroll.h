#pragma once

#include "Control.h"
#include "Types.h"

class ChatMessage;

class ChatScroll : public Control
{
public:
	ChatScroll(Control* pParent);

	ChatMessage* AddMessage(string name, string message, bool isUser);

	void StartListening();
	void StopListening();

protected:
	void OnUpdate(float fDeltaTime) override;
	void OnRender(SDL_Renderer* pRenderer) override {};

	void Poll();

	ChatMessage* _pLastBotMessage;
	bool _bListening = false;
	float _fListenTimer = 0.0f;

};