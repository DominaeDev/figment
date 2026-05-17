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
		ChatMessage(LayoutElement* pParent, fig::chat::Role role, const fig::uuid& characterId, fig::string name, fig::chat::MessageType msgType, bool bShowAvatar);

		void SetName(StringCRef name);
		void SetMessage(fig::string text, bool complete = false);
		void SetColors(const ColorPair& colors);
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
		TexturedBorderRenderer* _pSpeechBubbleBG = nullptr;
		TexturedBorderRenderer* _pSpeechBubbleBorder = nullptr;

		fig::chat::Role _role = fig::chat::Role::Undefined;
		bool _bShowAvatar = false;
		bool _bActive = true;
		fig::chat::MessageType _messageType = fig::chat::MessageType::Undefined;

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
}