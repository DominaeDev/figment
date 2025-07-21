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


ChatMessage::ChatMessage(Control* pParent, string name, Role role, MessageType msgType, bool bShowAvatar, bool bShowName) : Control(pParent),
	_name(name),
	_messageType(msgType),
	_role(role),
	_bShowAvatar(bShowAvatar),
	_bShowName(bShowName)
{
	SDL_Color bgColor;
	SDL_Color borderColor;
	switch (msgType)
	{
	case MessageType::Dialogue:
	case MessageType::Action:
		bgColor = role == Role::User ? Colors::UserMessageBackground : Colors::BotMessageBackground;
		borderColor = role == Role::User ? Colors::UserMessageBorder: Colors::BotMessageBorder;
		break;
	default:
		bgColor = Colors::NarrationBackground;
		borderColor = Colors::NarrationBorder;
		break;
	}

	_bShowName &= !name.empty();

	_style = Style::Default;
	if (msgType == MessageType::Dialogue)
		_style |= Style::Dialogue;
	else if (msgType == MessageType::Action)
		_style |= Style::Action;

	if (role == Role::Bot)
		_style |= Style::Left;
	else if (role == Role::User)
	{
#if USER_RIGHT_ALIGNED
		_style |= Style::Right;
#else
		_style |= Style::Left;
#endif
#if USER_YOU
		name = "You";
#endif
	}

	bool bRight = (_style & Style::Right) == Style::Right;
	bool bDialogue = (_style & Style::Dialogue) == Style::Dialogue;

	if (_bShowAvatar)
	{
		Image* pPortrait = new Image(this, CharacterImageStore::GetTexture(role == Role::User ? "Default" : "Female1", ImageType::Portrait_Square));
		pPortrait->SetSize(52, 52);
		pPortrait->SetPosition(bRight ? Constants::ChatScrollWidth - pPortrait->GetWidth() : 0, 0);
	}

	_pMessagePanel = new Panel(this);

	NineGridBackgroundRenderer* pBackground;

	if ((_style & Style::Dialogue) == Style::Dialogue)
	{
		if (bRight)
		{
			pBackground = new NineGridBackgroundRenderer({ 30, 72, 64, 30 });
			pBackground->SetTextures(TextureStore::GetTexture(Texture::SPEECH_BUBBLE_RIGHT_BG), TextureStore::GetTexture(Texture::SPEECH_BUBBLE_RIGHT_BORDER));
		}
		else
		{
			pBackground = new NineGridBackgroundRenderer({ 72, 30, 64, 30 });
			pBackground->SetTextures(TextureStore::GetTexture(Texture::SPEECH_BUBBLE_LEFT_BG), TextureStore::GetTexture(Texture::SPEECH_BUBBLE_LEFT_BORDER));
		}
	}
	else
	{
		pBackground = new NineGridBackgroundRenderer({ 30, 30, 64, 30 });
		pBackground->SetTextures(TextureStore::GetTexture(Texture::SPEECH_BUBBLE_CENTER_BG), TextureStore::GetTexture(Texture::SPEECH_BUBBLE_CENTER_BORDER));
	}
	_pMessagePanel->SetPosition(LEFT_MARGIN - (bDialogue ? DIALOGUE_OFFSET : 0), _bShowName ? TOP_OFFSET : 0); // Left

	pBackground->SetCornerSize(7.0f);
	pBackground->SetColors(bgColor, borderColor);

	_pMessagePanel->SetBackgroundRenderer(pBackground);
	_pMessagePanel->SetBackgroundColor(bgColor); // Text background

	SetSize(-1, 38);

	FontFace font = FontFace::Default;
	if (msgType != MessageType::Dialogue)
		font = FontFace::Italic;

	SDL_Color textColor = Colors::Black;
	if (!bDialogue)
		textColor = color_util::multiply_rgb(borderColor, 0.5f);

	_pMessageText = new StaticText(_pMessagePanel, "", font, Constants::ChatMessageFontSize, true);
	_pMessageText->SetPosition(TEXT_LEFT_MARGIN + (bDialogue && !bRight ? DIALOGUE_OFFSET : 0), 8);
	_pMessageText->SetForegroundColor(textColor);
	_pMessageText->SetBackgroundColor(Colors::Transparent);
	_pMessageText->SetMaxSize(toF(Constants::ChatScrollWidth - HMARGIN - TEXT_HMARGIN - 2), -1);

	// Name label
	if (_bShowName)
	{
		SDL_Color nameColor = color_util::add_rgb(borderColor, -0.1f);
		_pNameText = new StaticText(this, name, FontFace::NunitoBold, Constants::CharacterNameFontSize, false);
		_pNameText->SetAlignment(bRight ? TextAlignment::Right_Top : TextAlignment::Default);
		_pNameText->SetForegroundColor(nameColor);
		_pNameText->SetBackgroundColor(Colors::Transparent);
		_pNameText->SetPosition(LEFT_MARGIN, -1);
		_pNameText->SetSize(Constants::ChatScrollWidth - HMARGIN, -1);
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
		SetHeight(std::max(_pMessagePanel->GetHeight() + (_bShowName ? TOP_OFFSET : 0) + BOTTOM_MARGIN, MIN_HEIGHT));
	}
	
	int currentWidth = toI(_pMessagePanel->GetWidth());
	if (currentWidth < w)
	{
		_pMessagePanel->SetWidth(std::clamp(toF(w), MIN_WIDTH, Constants::ChatScrollWidth - HMARGIN));
		if (bRight)
			_pMessagePanel->SetX(Constants::ChatScrollWidth - _pMessagePanel->GetWidth() - RIGHT_MARGIN + (bDialogue ? DIALOGUE_OFFSET : 0));
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
	SetForegroundColor(SDL_Color { 0, 0, 0, (uint8_t)(bActive ? 255 : 160) });
}