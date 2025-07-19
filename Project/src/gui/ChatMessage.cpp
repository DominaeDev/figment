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
#include "util/Utility.h"
#include "Constants.h"

#define LEFT_MARGIN 68.0f
#define RIGHT_MARGIN 4.0f
#define MIN_HEIGHT 60.0f
#define TOP_OFFSET 20.0f
#define BOTTOM_MARGIN 2.0f
#define HMARGIN (LEFT_MARGIN + RIGHT_MARGIN)
#define VMARGIN (TOP_OFFSET + BOTTOM_MARGIN)

ChatMessage::ChatMessage(Control* pParent, string name, string message, Role role, MessageType msgType) : Control(pParent),
	_name(name),
	_messageType(msgType),
	_role(role)
{
	SDL_Color bgColor;
	SDL_Color borderColor;
	switch (msgType)
	{
	case MessageType::Dialogue:
	case MessageType::Action:
		bgColor = role == Role::User ? Color::UserMessageBackground : Color::BotMessageBackground;
		borderColor = role == Role::User ? Color::UserMessageBorder: Color::BotMessageBorder;
		break;
	default:
		bgColor = Color::NarrationBackground;
		borderColor = Color::NarrationBorder;
		break;
	}

	Image* pPortrait = new Image(this, CharacterImageStore::GetTexture(role == Role::User ? "Default" : "Female1", ImageType::Portrait_Square));
	pPortrait->SetSize(52, 52);

	_pMessagePanel = new Panel(this);

	auto pBackground = new NineGridBackgroundRenderer({ 72, 30, 64, 30 });
	pBackground->SetCornerSize(7.0f);
	pBackground->SetColors(bgColor, borderColor);
	pBackground->SetTextures(TextureStore::GetTexture(Texture::SPEECH_BUBBLE_BG), TextureStore::GetTexture(Texture::SPEECH_BUBBLE_BORDER));
	_pMessagePanel->SetBackgroundRenderer(pBackground);
	
	_pMessagePanel->SetBackgroundColor(bgColor); // Text background

	SetSize(-1, 38);

	_pStaticText = new StaticText(_pMessagePanel, message, FontFace::Default, Constants::ChatMessageFontSize, true);
	_pStaticText->SetPosition(32, 8);
	_pStaticText->SetBackgroundColor(Color::Transparent);

	SetMessage(message);
}

void ChatMessage::OnRender(SDL_Renderer* pRenderer)
{
	//DrawBackground(pRenderer);
}

void ChatMessage::SetMessage(const string& text)
{
	int w, h;
	_message = string_util::trim(text);
	_pStaticText->SetTextAndResize(_message, w, h);

	// Resize
	int currentHeight = toI(_pMessagePanel->GetHeight());
	if (currentHeight < h + 16)
	{
		_pMessagePanel->SetHeight(toF(h + 16));
		SetHeight(std::max(_pMessagePanel->GetHeight() + VMARGIN, MIN_HEIGHT));
		InvalidateParentLayout(true);
	}
}

void ChatMessage::AppendMessage(const string& text, bool lastPiece)
{
	if (_pStaticText)
		SetMessage(_pStaticText->GetText() + text);
}

void ChatMessage::OnSize()
{
	Control::OnSize();

	if (_bIgnoreEvent)
		return;

	if (_pMessagePanel)
	{
		_pMessagePanel->SetPosition(LEFT_MARGIN, TOP_OFFSET);
		_pMessagePanel->SetWidth(GetWidth() - HMARGIN);
	}
	if (_pStaticText)
		_pStaticText->SetMaxSize(_pMessagePanel->GetWidth() - 20 - HMARGIN, -1);
}