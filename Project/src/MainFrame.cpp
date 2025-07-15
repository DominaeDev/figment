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
	if (_bAutoChat)
		RunAutomation();
	
	if (_bStartedChat)
	{
		_bStartedChat = false;
		auto pLLM = Application::GetLLM();
		if (pLLM->GreetUser())
			_pChatScroll->StartListening();
	}

	// Poll llm status
	_fPollingCounter += fDeltaTime;
	if (_fPollingCounter > 0.1f)
		PollStatus();
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

	if (!pLLM->HasLoadedModel())
	{
		pLLM->LoadModelAsync(DEFAULT_MODEL_LOCATION, 
			[](int percent) 
			{
				SetStatusBar(std::format("Loading model... {0}%", percent));
			},
			[this](bool bSuccess) 
			{
				if (bSuccess)
				{
					SetStatusBar("Model loaded");
					DebugPrintLn("Loaded model OK");

					StartChat();
				}
				else
				{
					DebugPrintLn("Failed to load model");
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

void MainFrame::StartChat()
{
	auto pLLM = Application::GetLLM();
	if (pLLM && pLLM->HasLoadedModel())
	{
		string system_prompt = ReadTextFile("./resources/prompt_system.txt").value_or("");
		if (system_prompt.empty())
			DebugPrintLn(">> WARNING: No system prompt!");
		string formatting_spec = ReadTextFile("./resources/prompt_formatting.txt").value_or("");
		if (formatting_spec.empty())
			DebugPrintLn(">> WARNING: No formatting spec!");
		replace(system_prompt, "##FORMATTING_SPEC##", formatting_spec);
		pLLM->InitializeChat(system_prompt, {});

		_bStartedChat = true;
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
	if (!pLLM)
		return;

	switch (cmd.type)
	{
	case CommandType::UserMessage:
	{
		string formatted = FormatMessage(cmd.text, "{{user}}");
		_pChatScroll->AddMessage("User", cmd.text, MessageType::UserMessage);
		if (pLLM->SendMessage(Role::User, formatted))
			_pChatScroll->StartListening();
		break;
	}
	case CommandType::SystemMessage:
		if (pLLM->PushMessage(Role::System, cmd.text))
			_pChatScroll->AddMessage("System", "<" + cmd.text + ">", MessageType::SystemMessage);
		break;
	case CommandType::InstigateDialogue:
		if (pLLM->Instigate(Responder::Bot, MessageType::Dialogue, 1))
			_pChatScroll->StartListening();
		break;
	case CommandType::InstigateAction:
		if (pLLM->Instigate(Responder::Bot, MessageType::Action, 1))
			_pChatScroll->StartListening();
		break;
	case CommandType::PassTurn:
		if (pLLM->Instigate(Responder::Bot, MessageType::Undefined, 0))
			_pChatScroll->StartListening();
		break;
	case CommandType::Narrate:
		if (cmd.text.empty())
		{
			if (pLLM->Instigate(Responder::Narrator, MessageType::Narration, 1))
				_pChatScroll->StartListening();
		}
		else
		{
			string text = FormatMessage("["+cmd.text+"]", "");
			if (pLLM->PushMessage(Role::Narrator, text))
				_pChatScroll->AddMessage("Narrator", cmd.text, MessageType::Narration);
		}
		break;
	case CommandType::Revert:
	{
		int n = atoi(cmd.text.c_str());
		pLLM->RemoveMessages(std::max(n, 1));
		break;
	}
	case CommandType::Regenerate:
		if (pLLM->RemoveMessages(1) && pLLM->Instigate(Responder::Bot, MessageType::Undefined, 0))
			_pChatScroll->StartListening();
		break;
	case CommandType::Reset:
	{
		uint32_t seed = (uint32_t)atoi(cmd.text.c_str());
		if (seed != 0)
			pLLM->Reseed(seed);
		pLLM->ResetChat();
		break;
	}
	case CommandType::Reseed:
	{
		uint32_t seed = (uint32_t)atoi(cmd.text.c_str());
		if (seed == 0)
			seed = 0xFFFFFFFF;
		pLLM->Reseed(seed);
		break;
	}
	}
}

void MainFrame::ToggleAutoChat()
{
	_bAutoChat = !_bAutoChat;
}

static std::vector<std::string> split(std::string s, const std::string& delimiter)
{
	std::vector<std::string> tokens;
	size_t pos = 0;
	std::string token;
	while ((pos = s.find(delimiter)) != std::string::npos)
	{
		token = s.substr(0, pos);
		tokens.push_back(token);
		s.erase(0, pos + delimiter.length());
	}
	tokens.push_back(s);

	return tokens;
}

void MainFrame::RunAutomation()
{
	auto pLLM = Application::GetLLM();
	if (!pLLM)
		return;

	if (!pLLM->HasLoadedModel())
	{
		if (!pLLM->IsLoadingModel())
			LoadModel();
		return;
	}

	if (!pLLM->IsReady() || pLLM->IsGenerating())
		return;

	if (_autoScript.empty())
	{
		if (auto script = ReadTextFile("resources/auto_script.txt"))
		{
			string text = script.value();
			replace_all(text, "{{user}}", pLLM->GetUserName());
			replace_all(text, "{{char}}", pLLM->GetBotName());
			_autoScript = split(text, "\n");
		}
		_autoScriptIndex = 0;
	}

	if (_autoScript.empty())
	{
		_bAutoChat = false;
		return;
	}

	string message = _autoScript[_autoScriptIndex];
	_autoScriptIndex = ++_autoScriptIndex % _autoScript.size();

	string formatted = FormatMessage(message, "{{user}}");
	_pChatScroll->AddMessage("User", message, MessageType::UserMessage);
	if (pLLM->SendMessage(Role::User, formatted))
		_pChatScroll->StartListening();
}

void MainFrame::PollStatus()
{
	auto pLLM = Application::GetLLM();
	if (pLLM)
	{
		auto status = pLLM->GetStatus();
		if (status.bInvalid)
		{
			pLLM->Shutdown();
			_bAutoChat = false;
			_pStatusBar->SetMessage("Error occurred. Model unloaded.");
			_pStatusBar->SetModelInfo("", 0, 0);
			return;
		}

		if (status.bReady)
			_pStatusBar->SetModelInfo(status.modelName, status.allocCtxSize, status.usedCtxSize);
	}
}