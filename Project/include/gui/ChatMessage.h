#ifndef CHAT_MESSAGE_H__
#define CHAT_MESSAGE_H__

#pragma once

#include "llm/LLMTypes.h"
#include "gui/Control.h"
#include "gui/Graphics.h"

class StaticText;
class NineGridBackgroundRenderer;

class ChatMessage : public Control
{
public:
	ChatMessage(Control* pParent, Role role, string characterId, string name, MessageType msgType, bool bShowAvatar);

	void SetMessage(string text, bool complete = false);
	void SetColors(std::pair<Color, Color> colors);
	void SetColors(Color bgColor, Color borderColor);
	void AppendMessage(const string& text, bool complete = false);

	void SetActive(bool bActive);

protected:
	void RefreshColors();

private:
	string _name;
	string _message;
	string _characterId;

	Panel* _pMessagePanel;
	StaticText* _pMessageText = nullptr;
	StaticText* _pNameText = nullptr;
	NineGridBackgroundRenderer* _pSpeechBubbleBG;

	Role _role = Role::Undefined;
	bool _bShowAvatar = false;
	bool _bActive = true;
	MessageType _messageType = MessageType::Undefined;

	Color _bgColor;
	Color _borderColor;
	Color _nameColor;
	Color _textColor;

	enum Style
	{
		Default = 0,
		Dialogue = 1 << 0,
		Action = 1 << 1,
		System = 1 << 2,

		Left = 1 << 3,
		Right = 1 << 4,
	};
	int _style = Style::Default;
};

#endif