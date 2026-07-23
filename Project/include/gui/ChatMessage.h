#pragma once

#include "chat/ChatTypes.h"
#include "gui/Control.h"
#include "gui/GUITypes.h"

namespace fig::gui
{
	class Panel;
	class StaticText;
	class TexturedBorderRenderer;

	class ChatMessage : public Control
	{
	public:
		ChatMessage(ParentPtr pParent, fig::chat::Role role, const fig::uuid& characterId, fig::string name, fig::chat::MessageType msgType, bool bShowAvatar);

		void SetName(string_cref name);
		void SetMessage(fig::string text, bool complete = false);
		void SetColors(const fig::color_pair& colors);
		void SetColors(fig::color bgColor, fig::color borderColor);
		void AppendMessage(const fig::string& text, bool complete = false);

		void SetActive(bool bActive);

	protected:
		void RefreshColors();

	private:
		fig::string _name;
		fig::string _message;
		fig::string _characterId;

		fig::observer_ptr<Panel> _pMessagePanel;
		fig::observer_ptr<StaticText> _pMessageText;
		fig::observer_ptr<StaticText> _pNameText;
		fig::observer_ptr<TexturedBorderRenderer> _pSpeechBubbleBG;
		fig::observer_ptr<TexturedBorderRenderer> _pSpeechBubbleBorder;

		fig::chat::Role _role = fig::chat::Role::Undefined;
		bool _bShowAvatar = false;
		bool _bActive = true;
		fig::chat::MessageType _messageType = fig::chat::MessageType::Undefined;

		fig::color _bgColor {};
		fig::color _borderColor {};
		fig::color _nameColor {};
		fig::color _textColor {};

		enum Style
		{
			Default = 0,
			Dialogue = 1 << 0,
			Action = 1 << 1,
			System = 1 << 2,

			Left = 1 << 3,
			Right = 1 << 4,
			Name = 1 << 5,
		};
		int _style = Style::Default;
	};
}