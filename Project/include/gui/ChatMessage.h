#pragma once

#include "Types.h"
#include "Control.h"
#include "Graphics.h"

class StaticText;
class NineGridBackgroundRenderer;

class ChatMessage : public Control
{
public:
	ChatMessage(Control* pParent, string name, Role role, MessageType messageType, bool bShowAvatar, bool bShowName);

	void SetMessage(string text, bool complete = false);
	void AppendMessage(const string& text, bool complete = false);

	void SetActive(bool bActive);

protected:
	void RefreshColors();

private:
	string _name;
	string _message;
	
	Panel* _pMessagePanel;
	StaticText* _pMessageText = nullptr;
	StaticText* _pNameText = nullptr;
	NineGridBackgroundRenderer* _pSpeechBubbleBG;

	Role _role = Role::Undefined;
	bool _bShowAvatar = false;
	bool _bShowName = false;
	bool _bActive = true;
	MessageType _messageType = MessageType::Undefined;

	enum Style
	{
		Default = 0,
		Dialogue = 1 << 0,
		Action = 1 << 1,
		Left = 1 << 2,
		Right = 1 << 3,
	};
	int _style = Style::Default;
};