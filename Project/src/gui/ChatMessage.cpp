#include "gui/ChatMessage.h"

#include "gui/StaticText.h"
#include "gui/Panel.h"
#include "gui/Image.h"
#include "gui/NineGridBackgroundRenderer.h"
#include "gui/Color.h"
#include "gui/Fonts.h"
#include "gui/CustomRenderer.h"
#include "gui/TextureStore.h"
#include "gui/CharacterImageStore.h"
#include "util/StringUtility.h"
#include "Constants.h"
#include <format>

#define USER_RIGHT_ALIGNED	1
#define USER_YOU			1

#define LEFT_MARGIN			80.0f
#if USER_RIGHT_ALIGNED
#define RIGHT_MARGIN		80.0f
#else
#define RIGHT_MARGIN		0.0f
#endif
#define HMARGIN				(LEFT_MARGIN + RIGHT_MARGIN)
#define TOP_OFFSET			20.0f
#define BOTTOM_MARGIN		2.0f
#define MIN_WIDTH			40.0f
#define MIN_HEIGHT			40.0f

#define NAME_OFFSET			20.0f
#define DIALOGUE_OFFSET		15.0f
#define TEXT_LEFT_MARGIN	16.0f
#define TEXT_RIGHT_MARGIN	18.0f
#define TEXT_TOP_MARGIN		8.0f
#define TEXT_BOTTOM_MARGIN	10.0f
#define TEXT_HMARGIN		(TEXT_LEFT_MARGIN + TEXT_RIGHT_MARGIN)
#define TEXT_VMARGIN		(TEXT_TOP_MARGIN + TEXT_BOTTOM_MARGIN)

ChatMessage::ChatMessage(Control* pParent, Role role, string characterId, string name, MessageType msgType, bool bShowAvatar) : Control(pParent),
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
#if USER_RIGHT_ALIGNED
		_style |= Style::Right;
#else
		_style |= Style::Left;
#endif
#if USER_YOU
		if (!name.empty())
			name = "You";
#endif
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

	_pMessagePanel->SetPosition(LEFT_MARGIN - (bDialogue ? DIALOGUE_OFFSET : 0), _name.empty() ? 0 : TOP_OFFSET); // Left
	_pMessagePanel->SetBackgroundRenderer(_pSpeechBubbleBG);

	SetSize(-1, 38);

	FontFace font = FontFace::Default;
	if (msgType != MessageType::Dialogue)
		font = FontFace::Italic;

	_pMessageText = new StaticText(_pMessagePanel, "", font, Constants::GUI::ChatMessageFontSize, true);
	_pMessageText->SetPosition(TEXT_LEFT_MARGIN + (bDialogue && !bRight ? DIALOGUE_OFFSET : 0), 8);
	_pMessageText->SetBackgroundColor(Colors::Transparent);
	_pMessageText->SetMaxSize(toF(Constants::GUI::ChatScrollWidth - HMARGIN - TEXT_HMARGIN - 2), -1);

	// Name label
	if (!name.empty())
	{
		_pNameText = new StaticText(this, name, FontFace::NunitoBold, Constants::GUI::CharacterNameFontSize, false);
		_pNameText->SetAlignment(bRight ? TextAlignment::Right_Top : TextAlignment::Default);
		_pNameText->SetBackgroundColor(Colors::Transparent);
		_pNameText->SetPosition(LEFT_MARGIN, -1);
		_pNameText->SetSize(Constants::GUI::ChatScrollWidth - HMARGIN, -1);
	}
}

static void strip_ends(string& text, MessageType msgType)
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

void ChatMessage::SetMessage(string text, bool complete)
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
	w += toI(TEXT_HMARGIN + (bDialogue ? DIALOGUE_OFFSET : 0));
	h += toI(TEXT_VMARGIN);

	// Resize/position bubble
	int currentHeight = toI(_pMessagePanel->GetHeight());
	if (currentHeight < h)
	{
		_pMessagePanel->SetHeight(toF(h));
		SetHeight(std::max(_pMessagePanel->GetHeight() + (_name.empty() ? 0 : TOP_OFFSET) + BOTTOM_MARGIN, MIN_HEIGHT));
	}
	
	int currentWidth = toI(_pMessagePanel->GetWidth());
	if (currentWidth < w)
	{
		_pMessagePanel->SetWidth(std::clamp(toF(w), MIN_WIDTH, Constants::GUI::ChatScrollWidth - HMARGIN));
		if (bRight)
			_pMessagePanel->SetX(Constants::GUI::ChatScrollWidth - _pMessagePanel->GetWidth() - RIGHT_MARGIN + (bDialogue ? DIALOGUE_OFFSET : 0));
	}
	InvalidateParentLayout(true);
	InvalidateLayout();
}

void ChatMessage::AppendMessage(const string& text, bool complete)
{
	if (_pMessageText)
		SetMessage(_message + text, complete);
}

void ChatMessage::SetActive(bool bActive)
{
	if (_bActive == bActive)
		return;
	_bActive = bActive;
	RefreshColors();
}

void ChatMessage::SetColors(std::pair<Color, Color> colors)
{
	SetColors(colors.first, colors.second);
}

void ChatMessage::SetColors(Color bgColor, Color borderColor)
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

void ChatMessage::RefreshColors()
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