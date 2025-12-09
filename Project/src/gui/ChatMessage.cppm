export module GUI.Controls.ChatMessage;
export import GUI.Control;

import Common;

import GUI.Controls.Image;
import GUI.Controls.Panel;
import GUI.Controls.StaticText;
import GUI.Text;

import NineGridBackgroundRenderer;
import TextureStore;
import CharacterImageStore;

import LLMTypes;
import Utility;

constexpr bool RightAlignUser = true;
constexpr bool UserYou = true;
constexpr float MarginLeft = 80.0f;
constexpr float MarginRight = 80.0f;
constexpr float MarginHorizontal = (MarginLeft + MarginRight);
constexpr float MarginTop= 20.0f;
constexpr float MarginBottom = 2.0f;
constexpr float MinWidth = 40.0f;
constexpr float MinHeight = 40.0f;
constexpr float OffsetName = 20.0f;
constexpr float OffsetDialogue = 15.0f;
constexpr float TextMarginLeft = 16.0f;
constexpr float TextMarginRight = 18.0f;
constexpr float TextMarginTop = 8.0f;
constexpr float TextMarginBottom = 10.0f;
constexpr float TextMarginHorizontal = (TextMarginLeft + TextMarginRight);
constexpr float TextMarginVertical = (TextMarginTop + TextMarginBottom);

// Static functions

void strip_ends(string& text, MessageType msgType)
{
	string begin, end;
	switch (msgType)
	{
	case MessageType::Dialogue:
		begin = "\"";
		end = "\"";
		break;
	case MessageType::Action:
		begin = "*";
		end = "*";
		break;
	case MessageType::Direction:
		begin = "{";
		end = "}";
		break;
	case MessageType::Narration:
		begin = "[";
		end = "]";
		break;
	case MessageType::Thought:
		begin = "(";
		end = ")";
		break;
	default:
		return;
	}

	while (string_util::ends_with(text, end))
		text = text.substr(0, text.length() - 1);
	while (string_util::begins_with(text, begin))
		text = text.substr(1);
}


export class ChatMessage : public Control
{
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

public:
	ChatMessage(Control* pParent, Role role, string characterId, string name, MessageType msgType, bool bShowAvatar) : Control(pParent),
		_name(name),
		_messageType(msgType),
		_role(role),
		_bShowAvatar(bShowAvatar)
	{
		_style = Style::Default;
		if (msgType == MessageType::Dialogue)
			_style |= Style::Dialogue;
		else if (msgType == MessageType::Action)
			_style |= Style::Action;
		else if (msgType == MessageType::SystemMessage)
			_style |= Style::System;

		if (is_bot(role))
			_style |= Style::Left;
		else if (role == Role::User)
		{
			if constexpr (RightAlignUser)
				_style |= Style::Right;
			else
				_style |= Style::Left;
			
			if constexpr (UserYou)
			{
				if (!name.empty())
					name = "You";
			}
		}

		bool bRight = (_style & Style::Right) == Style::Right;
		bool bDialogue = (_style & Style::Dialogue) == Style::Dialogue;

		if (_bShowAvatar)
		{
			Texture* pTexture = CharacterImageStore::GetTexture(characterId, ImageType::Portrait_Square);
			if (!pTexture)
				pTexture = CharacterImageStore::GetTexture("Default", ImageType::Portrait_Square);

			Image* pPortrait = new Image(this, pTexture);
			pPortrait->SetSize(52, 52);
			pPortrait->SetPosition(bRight ? Constants::GUI::ChatScrollWidth - pPortrait->GetWidth() : 0, 0);
		}

		_pMessagePanel = new Panel(this);

		if ((_style & Style::Dialogue) == Style::Dialogue)
		{
			if (bRight)
			{
				_pSpeechBubbleBG = new NineGridBackgroundRenderer({ 30, 72, 64, 30 });
				_pSpeechBubbleBG->SetTextures(TextureStore::GetTexture(TextureType::SPEECH_BUBBLE_RIGHT_BG), TextureStore::GetTexture(TextureType::SPEECH_BUBBLE_RIGHT_BORDER));
			}
			else
			{
				_pSpeechBubbleBG = new NineGridBackgroundRenderer({ 72, 30, 64, 30 });
				_pSpeechBubbleBG->SetTextures(TextureStore::GetTexture(TextureType::SPEECH_BUBBLE_LEFT_BG), TextureStore::GetTexture(TextureType::SPEECH_BUBBLE_LEFT_BORDER));
			}
		}
		else
		{
			_pSpeechBubbleBG = new NineGridBackgroundRenderer({ 30, 30, 64, 30 });
			_pSpeechBubbleBG->SetTextures(TextureStore::GetTexture(TextureType::SPEECH_BUBBLE_CENTER_BG), TextureStore::GetTexture(TextureType::SPEECH_BUBBLE_CENTER_BORDER));
		}
		_pSpeechBubbleBG->SetCornerSize(7.0f);

		_pMessagePanel->SetPosition(MarginLeft - (bDialogue ? OffsetDialogue : 0), _name.empty() ? 0 : MarginTop); // Left
		_pMessagePanel->SetBackgroundRenderer(_pSpeechBubbleBG);

		SetSize(-1, 38);

		FontFace font = FontFace::Default;
		if (msgType != MessageType::Dialogue)
			font = FontFace::Italic;

		_pMessageText = new StaticText(_pMessagePanel, "", font, Constants::GUI::ChatMessageFontSize, true);
		_pMessageText->SetPosition(TextMarginLeft + (bDialogue && !bRight ? OffsetDialogue : 0), 8);
		_pMessageText->SetBackgroundColor(Colors::Transparent);
		_pMessageText->SetMaxSize(toF(Constants::GUI::ChatScrollWidth - MarginHorizontal - TextMarginHorizontal - 2), -1);

		// Name label
		if (!name.empty())
		{
			_pNameText = new StaticText(this, name, FontFace::NunitoBold, Constants::GUI::CharacterNameFontSize, false);
			_pNameText->SetAlignment(bRight ? TextAlignment::Right_Top : TextAlignment::Default);
			_pNameText->SetBackgroundColor(Colors::Transparent);
			_pNameText->SetPosition(MarginLeft, -1);
			_pNameText->SetSize(Constants::GUI::ChatScrollWidth - MarginHorizontal, -1);
		}
	}

	void SetMessage(string text, bool complete = false)
	{
		bool bDialogue = (_style & Style::Dialogue) == Style::Dialogue;
		bool bRight = (_style & Style::Right) == Style::Right;

		strip_ends(text, _messageType);

		int w, h;
		_message = text;
		if (_messageType == MessageType::Dialogue)
		{
			text = "\u201C" + text;
			if (complete)
				text += "\u201D";
		}
		else if (_messageType == MessageType::Action)
		{
			text = "*" + text;
			if (complete)
				text += "*";
		}
		else if (_messageType == MessageType::Thought)
		{
			text = "(" + text;
			if (complete)
				text += ")";
		}

		_pMessageText->SetTextAndResize(string_util::trim(text), w, h);
		w += toI(TextMarginHorizontal + (bDialogue ? OffsetDialogue : 0));
		h += toI(TextMarginVertical);

		// Resize/position bubble
		int currentHeight = toI(_pMessagePanel->GetHeight());
		if (currentHeight < h)
		{
			_pMessagePanel->SetHeight(toF(h));
			SetHeight(std::max(_pMessagePanel->GetHeight() + (_name.empty() ? 0 : MarginTop) + MarginBottom, MinHeight));
		}

		int currentWidth = toI(_pMessagePanel->GetWidth());
		if (currentWidth < w)
		{
			_pMessagePanel->SetWidth(std::clamp(toF(w), MinWidth, Constants::GUI::ChatScrollWidth - MarginHorizontal));
			if (bRight)
				_pMessagePanel->SetX(Constants::GUI::ChatScrollWidth - _pMessagePanel->GetWidth() - (RightAlignUser ? MarginRight : 0.0f) + (bDialogue ? OffsetDialogue : 0));
		}
		InvalidateParentLayout(true);
		InvalidateLayout();
	}

	void SetColors(std::pair<Color, Color> colors)
	{
		SetColors(colors.first, colors.second);
	}

	void SetColors(Color bgColor, Color borderColor)
	{
		_bgColor = bgColor;
		_borderColor = borderColor;
		if ((_style & Style::Dialogue) == Style::Dialogue)
			_textColor = Colors::Black;
		else
			_textColor = color_util::multiply_rgb(borderColor, 0.5f);
		_nameColor = color_util::add_rgb(borderColor, -0.1f);
		RefreshColors();
	}

	void AppendMessage(const string& text, bool complete = false)
	{
		if (_pMessageText)
			SetMessage(_message + text, complete);
	}

	void SetActive(bool bActive)
	{
		if (_bActive == bActive)
			return;
		_bActive = bActive;
		RefreshColors();
	}

protected:
	void RefreshColors()
	{
		const uint8_t fadedAlpha = 120;
		bool bDialogue = (_style & Style::Dialogue) == Style::Dialogue;

		uint8_t alpha = (uint8_t)(_bActive ? 255 : fadedAlpha);

		SetForegroundColor(Color { 0, 0, 0, alpha });

		_pSpeechBubbleBG->SetColors(color_util::with_alpha(_bgColor, alpha), color_util::with_alpha(_borderColor, alpha));
		_pMessagePanel->SetBackgroundColor(color_util::with_alpha(_pMessagePanel->GetBackgroundColor(), alpha));
		_pMessageText->SetForegroundColor(color_util::with_alpha(_textColor, alpha));
		if (_pNameText)
			_pNameText->SetForegroundColor(color_util::with_alpha(_nameColor, alpha));
	}
};
