#include "MainFrame.h"
#include "Panel.h"
#include "StaticText.h"
#include "HorizontalSizer.h"
#include "VerticalSizer.h"
#include "TextBox.h"
#include "Constants.h"
#include "Color.h"
#include "ChatScroll.h"
#include "Utility.h"
#include "Inference.h"

MainFrame::MainFrame(SDL_Window* pWindow) : Frame(pWindow)
{
	SetForegroundColor(SDL_Color { 0, 0, 0, SDL_ALPHA_OPAQUE });
	SetBackgroundColor(SDL_Color { 30, 30, 30, SDL_ALPHA_OPAQUE });

	auto leftPanel = new Panel(this);
	leftPanel->SetSize(200, -1);

	auto centerPanel = new Panel(this);
	centerPanel->SetBackgroundColor(SDL_Color { 40, 40, 40, SDL_ALPHA_OPAQUE });
	centerPanel->SetSize(800, -1);

	auto rightPanel = new Panel(this);
	rightPanel->SetSize(200, -1);
	rightPanel->SetMinSize(200, -1);

	auto pStaticText = new StaticText(centerPanel, "Hello, how are you? iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii", FontFace::Default, Constants::DefaultFontSize);
	pStaticText->SetAlignment(TextAlignment::Middle_Center);
	pStaticText->SetSize(80, 80);
	pStaticText->SetMinSize(-1, 80);
	pStaticText->SetBackgroundColor(SDL_Color { 255, 255, 0, SDL_ALPHA_OPAQUE });
	pStaticText->SetVisible(false);

	auto pChatScroll = new ChatScroll(centerPanel);

	auto pTextBox = new TextBox(centerPanel, FontFace::Default, Constants::DefaultFontSize);
	pTextBox->SetSize(-1, 88);
	pTextBox->SetBackgroundColor(Color::White);
	pTextBox->SelectAll();

	auto pCenterSizer = new VerticalSizer();
	pCenterSizer->Add(pChatScroll, -1, Sizer::Expand);
	pCenterSizer->Add(pTextBox, 0, Sizer::AlignBottom | Sizer::Expand);
	centerPanel->SetSizer(pCenterSizer);

	auto pTopSizer = new HorizontalSizer();
	pTopSizer->Add(leftPanel, -1, Sizer::Expand);
	pTopSizer->Add(centerPanel, 0, Sizer::Expand | Sizer::Bottom, 8);
	pTopSizer->Add(rightPanel, -1, Sizer::Expand);
	SetSizer(pTopSizer);

	pTextBox->SetEnterPressedCallback([pChatScroll](string text) {
		if (!isEmpty(text))
		{
			pChatScroll->AddMessage("User", text);

			if (Inference::HasLoadedModel())
			{
				string response;
				if (Inference::SendMessage("User", text, response))
				{
					pChatScroll->AddMessage("Bot", response);
				}
			}
		}
	});

	pTextBox->SetFocus(true);
}

MainFrame::~MainFrame()
{
}

void MainFrame::OnUpdate(float fDeltaTime)
{
}

void MainFrame::OnRender(SDL_Renderer* pRenderer)
{
	DrawBackground(pRenderer);
}