#include "MainFrame.h"
#include "Utility.h"
#include "Area.h"
#include "Panel.h"
#include "StaticText.h"
#include "HorizontalSizer.h"
#include "VerticalSizer.h"
#include "TextBox.h"
#include "Constants.h"
#include "Color.h"
#include "ChatScroll.h"
#include "ChatMessage.h"
#include "StatusBar.h"
#include "StringUtil.h"
#include "LLMInstance.h"
#include "AppState.h"
#include <format>
#include "SolidBackgroundRenderer.h"
#include "RoundedBackgroundRenderer.h"
#include "NineGridBackgroundRenderer.h"
#include "RoundedBorderRenderer.h"
#include "CommandParser.h"
#include "Message.h"

MainFrame* MainFrame::s_pInstance = nullptr;

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

	_pChatScroll = new ChatScroll(centerPanel);

	auto pTextBox = new TextBox(centerPanel, FontFace::Default, Constants::DefaultFontSize);
	pTextBox->SetSize(-1, 88);
	pTextBox->SelectAll();

	auto pCenterSizer = new VerticalSizer();
	pCenterSizer->Add(_pChatScroll, -1, Sizer::Expand);
	pCenterSizer->Add(pTextBox, 0, Sizer::AlignBottom | Sizer::Expand);
	centerPanel->SetSizer(pCenterSizer);

	auto mainSizer = new HorizontalSizer();
	mainSizer->Add(leftPanel, -1, Sizer::Expand);
	mainSizer->Add(centerPanel, 0, Sizer::Expand | Sizer::Bottom | Sizer::Top, 8);
	mainSizer->Add(rightPanel, -1, Sizer::Expand);
	mainArea->SetSizer(mainSizer);

	// Status bar
	_pStatusBar = new StatusBar(this);

	auto topSizer = new VerticalSizer();
	topSizer->Add(mainArea, -1, Sizer::Expand);
	topSizer->Add(_pStatusBar, 0);

	SetSizer(topSizer);
	
	pTextBox->SetEnterPressedCallback([this](string text) {
		OnCommand(CommandParser::Parse(text));
	});

//	pTextBox->SetBorderRenderer(new RoundedBorderRenderer(4.5f, 2.5f, Color::Black));
	pTextBox->SetBackgroundRenderer(new NineGridBackgroundRenderer(10.0f, Color::White, Color::Black));

	pTextBox->SetFocus(true);

	InvalidateLayout();
	s_pInstance = this;
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

void MainFrame::LoadModel()
{
	SetStatusBar("Loading model...");

	auto pLLM = Application::GetLLM();
	if (!pLLM)
		return;

	pLLM->SetStatusCallback([this](LLMStatus status) {
		_pStatusBar->SetModelInfo(status.modelName, status.allocCtxSize, status.usedCtxSize);
	});

	if (!pLLM->HasLoadedModel())
	{
		pLLM->LoadModelAsync(DEFAULT_MODEL_LOCATION, 
			[](int percent) 
			{
				SetStatusBar(std::format("Loading model... {0}%", percent));
			},
			[](bool bSuccess) 
			{
				if (bSuccess)
				{
					SetStatusBar("Model loaded");
					fprintf(stdout, "Loaded model OK");
				}
				else
				{
					fprintf(stdout, "Failed to load model");
					SetStatusBar("Failed to load model");
				}
			});
	}
}

void MainFrame::UnloadModel()
{
	auto pLLM = Application::GetLLM();
	if (pLLM && pLLM->HasLoadedModel())
	{
		pLLM->Shutdown();
		SetStatusBar("Model unloaded");
	}
}

void MainFrame::SetStatusBar(string message)
{
	s_pInstance->_pStatusBar->SetMessage(message);
}

void MainFrame::OnCommand(Command cmd)
{
	if (cmd.type == CommandType::Invalid)
		return;

	auto pLLM = Application::GetLLM();
	if (pLLM)
	{
		if (cmd.type == CommandType::Say)
		{
			string formatted = FormatMessage(cmd.text, "User");
			_pChatScroll->AddMessage("User", cmd.text, MessageType::UserMessage);
			if (pLLM->SendMessage("User", formatted))
			{
				printf("%s\n", formatted.c_str());
				_pChatScroll->StartListening();
			}
		}
		else if (cmd.type == CommandType::SystemMessage)
		{
			_pChatScroll->AddMessage("System", "<" + cmd.text + ">", MessageType::SystemMessage);
			pLLM->SendMessage("system", cmd.text, false);
		}

	}
}