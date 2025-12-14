#pragma once

#include "model/ChatTypes.h"
#include "gui/Control.h"
#include "gui/Graphics.h"

class StaticText;
class NineGridBackgroundRenderer;

class ChatMessage : public Control
{
public:
	ChatMessage(Control* pParent, Role role, fig::string characterId, fig::string name, MessageType msgType, bool bShowAvatar);

	void SetMessage(fig::string text, bool complete = false);
	void SetColors(std::pair<Color, Color> colors);
	void SetColors(Color bgColor, Color borderColor);
	void AppendMessage(const fig::string& text, bool complete = false);

	void SetActive(bool bActive);

protected:
	void RefreshColors();

private:
	fig::string _name;
	fig::string _message;
	fig::string _characterId;

	Panel* _pMessagePanel;
	StaticText* _pMessageText = nullptr;
	StaticText* _pNameText = nullptr;
	NineGridBackgroundRenderer* _pSpeechBubbleBG;

	Role _role = Role::Undefined;
	bool _bShowAvatar = false;
	bool _bActive = true;
	MessageType _messageType = MessageType::Undefined;

	Color _bgColor {};
	Color _borderColor {};
	Color _nameColor {};
	Color _textColor {};

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