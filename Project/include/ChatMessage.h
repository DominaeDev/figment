#pragma once

#include "Control.h"
#include "Types.h"

class StaticText;

class ChatMessage : public Control
{
public:
	ChatMessage(Control* pParent, string name, string message);

	void StartListening();
	void StopListening();

protected:
	void OnUpdate(float fDeltaTime) override;
	void OnRender(SDL_Renderer* pRenderer) override;

	void Poll();
	void AppendText(const string& text);

private:
	string _name;
	string _message;
	StaticText* _pStaticText = nullptr;
	bool _bIgnoreEvent = false;
	bool _bListening = false;
	float _fListenTimer = 0.0f;
};