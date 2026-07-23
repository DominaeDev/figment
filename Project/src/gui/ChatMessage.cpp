#include <pch.h>
#include "gui/ChatMessage.h"

#include "gui/GUICommon.h"
#include "gui/CustomRenderers.h"

#include "gui/AppResources.h"
#include "app/AppState.h"
#include "user/UserManager.h"

#include <format>

#define USER_RIGHT_ALIGNED	1
#define USER_YOU			1

#define LEFT_MARGIN			80
#if USER_RIGHT_ALIGNED
#define RIGHT_MARGIN		80
#else
#define RIGHT_MARGIN		0.0f
#endif
#define HMARGIN				(LEFT_MARGIN + RIGHT_MARGIN)
#define TOP_OFFSET			20
#define BOTTOM_MARGIN		2
#define MIN_WIDTH			40
#define MIN_HEIGHT			40

#define NAME_OFFSET			20
#define DIALOGUE_OFFSET		15
#define TEXT_LEFT_MARGIN	16
#define TEXT_RIGHT_MARGIN	18
#define TEXT_TOP_MARGIN		8
#define TEXT_BOTTOM_MARGIN	10
#define TEXT_HMARGIN		(TEXT_LEFT_MARGIN + TEXT_RIGHT_MARGIN)
#define TEXT_VMARGIN		(TEXT_TOP_MARGIN + TEXT_BOTTOM_MARGIN)

using namespace fig::chat;

namespace fig::gui
{
	ChatMessage::ChatMessage(ParentPtr pParent, Role role, const fig::uuid& characterId, fig::string name, MessageType msgType, bool bShowAvatar) : Control(pParent),
		_name(name),
		_messageType(msgType),
		_role(role),
		_bShowAvatar(bShowAvatar),
		_bgColor {},
		_borderColor {},
		_nameColor {},
		_textColor {}
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
				name = Constants::Chat::Names::User;
#endif
		}
		if (!name.empty() and msgType != MessageType::SystemMessage)
			_style |= Style::Name;

		bool bRight = (_style & Style::Right) == Style::Right;
		bool bDialogue = (_style & Style::Dialogue) == Style::Dialogue;
		bool bShowName = (_style & Style::Name) == Style::Name;

		if (_bShowAvatar)
		{
			fig::texture_ptr pTexture = nullptr;
			if (auto try_portrait = Global::GetUserContent().GetSmallPortraitForCharacter(characterId, AppResources::GetTexture(Resource::MASK_SMALL_PORTRAIT_56PX), GetSDLRenderer()))
				pTexture = (*try_portrait).get();

			if (!pTexture)
				pTexture = AppResources::GetTexture(Resource::PROFILE_DEFAULT_IMAGE);

			Image* pPortrait = CreateControl<Image>(pTexture);
			pPortrait->SetSize(Constants::Chat::SmallPortraitWidth, Constants::Chat::SmallPortraitWidth);

			constexpr int32_t spacing = 24;
			if (bRight)
				pPortrait->SetX(Constants::GUI::ChatScrollWidth - RIGHT_MARGIN + spacing);
			else
				pPortrait->SetX(LEFT_MARGIN - Constants::Chat::SmallPortraitWidth - spacing);
		}

		_pMessagePanel = CreateControl<Panel>();

		if (bDialogue)
		{
			if (bRight)
			{
				_pSpeechBubbleBG = _pMessagePanel->SetBackgroundRenderer<TexturedBorderRenderer>(Resource::SPEECH_BUBBLE_RIGHT_BG, fig::corners { 30, 72, 64, 30 });
				_pSpeechBubbleBorder = _pMessagePanel->SetBorderRenderer<TexturedBorderRenderer>(Resource::SPEECH_BUBBLE_RIGHT_BORDER, fig::corners { 30, 72, 64, 30 });
			}
			else
			{
				_pSpeechBubbleBG = _pMessagePanel->SetBackgroundRenderer<TexturedBorderRenderer>(Resource::SPEECH_BUBBLE_LEFT_BG, fig::corners { 72, 30, 64, 30 });
				_pSpeechBubbleBorder = _pMessagePanel->SetBorderRenderer<TexturedBorderRenderer>(Resource::SPEECH_BUBBLE_LEFT_BORDER, fig::corners { 72, 30, 64, 30 });
			}
		}
		else
		{
			_pSpeechBubbleBG = _pMessagePanel->SetBackgroundRenderer<TexturedBorderRenderer>(Resource::SPEECH_BUBBLE_CENTER_BG, fig::corners { 30, 30, 64, 30 });
			_pSpeechBubbleBorder = _pMessagePanel->SetBorderRenderer<TexturedBorderRenderer>(Resource::SPEECH_BUBBLE_CENTER_BORDER, fig::corners { 30, 30, 64, 30 });
		}
		_pSpeechBubbleBG->SetCornerScale(0.35f);
		_pSpeechBubbleBorder->SetCornerScale(0.35f);
		_pSpeechBubbleBG->SetExtend(5.0f);
		_pSpeechBubbleBorder->SetExtend(5.0f);

		_pMessagePanel->SetPosition(LEFT_MARGIN - (bDialogue ? DIALOGUE_OFFSET : 0), bShowName ? TOP_OFFSET : 0); // Left

		SetSize(-1, 38);

		FontFace font = FontFace::Default;
		if (msgType != MessageType::Dialogue)
			font = FontFace::Italic;

		_pMessageText = _pMessagePanel->CreateControl<StaticText>("", font, Constants::GUI::ChatMessageFontSize, true);
		_pMessageText->EnableWordWrap(true);
		_pMessageText->SetPosition(TEXT_LEFT_MARGIN + (bDialogue && !bRight ? DIALOGUE_OFFSET : 0), 8);
		_pMessageText->SetBackgroundColor(Colors::Transparent);
		_pMessageText->SetMaxSize(Constants::GUI::ChatScrollWidth - HMARGIN - TEXT_HMARGIN - 2, -1);

		// Name label
		if ((_style & Style::Name) == Style::Name)
		{
			_pNameText = CreateControl<StaticText>(name, FontFace::NunitoBold, Constants::GUI::CharacterNameFontSize, false);
			_pNameText->SetAlignment(bRight ? TextAlignment::Right_Top : TextAlignment::Default);
			_pNameText->SetBackgroundColor(Colors::Transparent);
			_pNameText->SetPosition(LEFT_MARGIN, -1);
			_pNameText->SetSize(Constants::GUI::ChatScrollWidth - HMARGIN, -1);
		}
	}

	static void strip_ends(fig::string& text, MessageType msgType)
	{
		fig::string begin, end;
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

		while (ends_with(text, end))
			text = text.substr(0, text.length() - 1);
		while (begins_with(text, begin))
			text = text.substr(1);
	}

	void ChatMessage::SetName(string_cref name)
	{
		_name = name;
	}

	void ChatMessage::SetMessage(fig::string text, bool complete)
	{
		bool bDialogue = (_style & Style::Dialogue) == Style::Dialogue;
		bool bRight = (_style & Style::Right) == Style::Right;
		bool bShowName = (_style & Style::Name) == Style::Name;

		strip_ends(text, _messageType);

		int32_t w, h;
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

		_pMessageText->SetTextAndResize(trim(text), w, h);
		w += TEXT_HMARGIN + (bDialogue ? DIALOGUE_OFFSET : 0);
		h += TEXT_VMARGIN;

		// Resize/position bubble
		int currentHeight = toI(_pMessagePanel->GetHeight());
		if (currentHeight < h)
		{
			_pMessagePanel->SetHeight(h);
			SetHeight(std::max(_pMessagePanel->GetHeight() + (bShowName ? TOP_OFFSET : 0) + BOTTOM_MARGIN, MIN_HEIGHT));
		}

		int currentWidth = toI(_pMessagePanel->GetWidth());
		if (currentWidth < w)
		{
			_pMessagePanel->SetWidth(std::clamp(w, MIN_WIDTH, Constants::GUI::ChatScrollWidth - HMARGIN));
			if (bRight)
				_pMessagePanel->SetX(Constants::GUI::ChatScrollWidth - _pMessagePanel->GetWidth() - RIGHT_MARGIN + (bDialogue ? DIALOGUE_OFFSET : 0));
		}
		InvalidateParentLayout(true);
		InvalidateLayout();
	}

	void ChatMessage::AppendMessage(const fig::string& text, bool complete)
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

	void ChatMessage::SetColors(const fig::color_pair& colors)
	{
		SetColors(colors.background, colors.foreground);
	}

	void ChatMessage::SetColors(fig::color bgColor, fig::color borderColor)
	{
		_bgColor = bgColor;
		_borderColor = borderColor;
		if ((_style & Style::Dialogue) == Style::Dialogue)
			_textColor = Colors::Black;
		else
			_textColor = borderColor.Multiply(0.5f);
		_nameColor = borderColor.Add(-0.1f);
		RefreshColors();
	}

	void ChatMessage::RefreshColors()
	{
		const uint8_t fadedAlpha = 120;
		bool bDialogue = (_style & Style::Dialogue) == Style::Dialogue;
		fig::color chatBG = GetBackgroundColor();

		uint8_t alpha = (uint8_t)(_bActive ? 255 : fadedAlpha);

		SetForegroundColor(fig::color { 0, 0, 0, alpha });

		_pSpeechBubbleBG->SetColor(_bgColor.WithAlpha(alpha));
		_pSpeechBubbleBorder->SetColor(_borderColor.WithAlpha(alpha));

		_pMessagePanel->SetBackgroundColor(chatBG.WithAlpha(alpha));
		_pMessageText->SetForegroundColor(_textColor.WithAlpha(alpha));
		
		if (_bActive)
		{
			_pMessageText->SetBackgroundColor(_bgColor);
			if (_pNameText)
				_pNameText->SetBackgroundColor(chatBG);
		}
		else
		{
			_pMessageText->SetBackgroundColor(Colors::Transparent);
			if (_pNameText)
				_pNameText->SetBackgroundColor(Colors::Transparent);
		}

		if (_pNameText)
			_pNameText->SetForegroundColor(_nameColor.WithAlpha(alpha));
	}
}