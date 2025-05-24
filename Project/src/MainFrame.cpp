#include "MainFrame.h"
#include "Area.h"
#include "Panel.h"
#include "StaticText.h"
#include "HorizontalSizer.h"
#include "VerticalSizer.h"
#include "TextBox.h"
#include "Constants.h"
#include "Color.h"
#include "ChatScroll.h"
#include "Utility.h"
#include "LLMInstance.h"
#include "AppState.h"

MainFrame::MainFrame(SDL_Window* pWindow) : Frame(pWindow)
{
	SetForegroundColor(SDL_Color { 0, 0, 0, SDL_ALPHA_OPAQUE });
	SetBackgroundColor(SDL_Color { 30, 30, 30, SDL_ALPHA_OPAQUE });

	auto mainArea = new Area(this);

	auto leftPanel = new Panel(mainArea);
	leftPanel->SetSize(200, -1);

	auto centerPanel = new Panel(mainArea);
	centerPanel->SetBackgroundColor(SDL_Color { 40, 40, 40, SDL_ALPHA_OPAQUE });
	centerPanel->SetSize(800, -1);

	auto rightPanel = new Panel(mainArea);
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

	auto mainSizer = new HorizontalSizer();
	mainSizer->Add(leftPanel, -1, Sizer::Expand);
	mainSizer->Add(centerPanel, 0, Sizer::Expand | Sizer::Bottom | Sizer::Top, 8);
	mainSizer->Add(rightPanel, -1, Sizer::Expand);
	mainArea->SetSizer(mainSizer);

	auto pStatusBar = new StaticText(this, "Hello", FontFace::Default, Constants::StatusBarFontSize, false);
	pStatusBar->SetSize(this->GetWidth(), 24);
	pStatusBar->SetForegroundColor(SDL_Color { 200, 200, 200, SDL_ALPHA_OPAQUE });
	pStatusBar->SetBackgroundColor(SDL_Color { 40, 40, 40, SDL_ALPHA_OPAQUE });
	pStatusBar->SetMargins(8, 0, 0, 0);

	auto topSizer = new VerticalSizer();
	topSizer->Add(mainArea, -1, Sizer::Expand);
	topSizer->Add(pStatusBar, 0, Sizer::Expand);

	SetSizer(topSizer);
	
	pTextBox->SetEnterPressedCallback([pChatScroll](string text) {
		if (!isEmpty(text))
		{
			pChatScroll->AddMessage("User", text);

			auto pLLM = Application::GetLLM();
			string response;
			if (pLLM && pLLM->EnqueueMessage("User", text, response))
			{
				pChatScroll->AddMessage("Bot", response);
			}
		}
	});

	pTextBox->SetFocus(true);

	InvalidateLayout();
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

#define DEFAULT_MODEL_LOCATION "M:\\Backyard\\ana-v1-m7.erp.unc.Q6_K.gguf"

void MainFrame::LoadModel()
{
	auto pLLM = Application::GetLLM();
	if (pLLM && !pLLM->HasLoadedModel())
	{
		pLLM->LoadModelAsync(DEFAULT_MODEL_LOCATION, [](bool bOk) {
			fprintf(stdout, "%s", bOk? "Loaded model OK" : "Failed to load model");
		});
	}
}