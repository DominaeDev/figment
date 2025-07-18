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
	if (_bStartedChat)
	{
		_bStartedChat = false;
		auto pLLM = Application::GetLLM();
		if (pLLM)
			pLLM->GreetUser();
	}

	// Poll llm status
	_fPollingCounter += fDeltaTime;
	if (_fPollingCounter > 0.1f)
		PollStatus();

#if AUTOCHAT
	if (_bAutoChat) AutoChat();
#endif
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

#if AUTOCHAT
		_bAutoChat = false;
#endif
	}
}

void MainFrame::StartChat()
{
	auto pLLM = Application::GetLLM();
	if (pLLM && pLLM->HasLoadedModel())
	{
		string system_prompt = ReadTextFile("./resources/prompting/prompt_system.txt").value_or("");
		if (system_prompt.empty())
			DebugPrintLn(">> WARNING: No system prompt!");
		string formatting_spec = ReadTextFile("./resources/prompting/prompt_formatting.txt").value_or("");
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
		pLLM->SendMessage(cmd.text);
		break;
	case CommandType::SystemMessage:
		pLLM->PushMessage(Role::System, cmd.text, MessageType::SystemMessage);
		break;
	case CommandType::InstigateDialogue:
		pLLM->InstigateResponse(Responder::Bot, MessageType::Dialogue, 1);
		break;
	case CommandType::InstigateAction:
		pLLM->InstigateResponse(Responder::Bot, MessageType::Action, 1);
		break;
	case CommandType::PassTurn:
		pLLM->InstigateResponse(Responder::Bot, MessageType::Undefined);
		break;
	case CommandType::Impersonate:
		pLLM->InstigateResponse(Responder::User, MessageType::Dialogue, 1);
		break;
	case CommandType::Narrate:
		if (cmd.text.empty())
			pLLM->InstigateResponse(Responder::Narrator, MessageType::Narration, 1);
		else
			pLLM->PushMessage(Role::Narrator, "[" + cmd.text + "]", MessageType::Narration);
		break;
	case CommandType::Instruct:
		if (!cmd.text.empty())
			pLLM->Instruct(cmd.text);
		break;
	case CommandType::RemoveLast:
	{
		int n = atoi(cmd.text.c_str());
		auto removedIds = pLLM->RemoveMessages(std::max(n, 1));
		_pChatScroll->RemoveMessages(removedIds);
		break;
	}
	case CommandType::RedoResponse:
	{
		auto removedIds = pLLM->RemoveMessages(1);
		_pChatScroll->RemoveMessages(removedIds);
		pLLM->InstigateResponse(Responder::Bot, MessageType::Undefined, 0);
		break;
	}
	case CommandType::RollbackUserMessage:
	{
		auto removedIds = pLLM->RollbackUserMessage();
		_pChatScroll->RemoveMessages(removedIds);
		break;
	}
	case CommandType::Reset:
	{
		uint32_t seed = (uint32_t)atoi(cmd.text.c_str());
		if (pLLM->ResetChat(seed))
			_pChatScroll->ClearMessages();
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
	case CommandType::Look:
		if (!cmd.text.empty())
		{
			pLLM->PushMessage(Role::Narrator, "[{{user}} takes a moment to examine " + cmd.text + ".]", MessageType::Narration, false, 1);
			pLLM->PushMessage(Role::Director, "{{Describe what {{user}} can clearly see of " + cmd.text + ", while paying extra attention to detail.}}", MessageType::Direction, false, 1);
		}
		else 
		{
			pLLM->PushMessage(Role::Narrator, "[{{user}} takes a moment to observe their surroundings.]", MessageType::Narration, false, 1);
			pLLM->PushMessage(Role::Director, "{{Describe what {{user}} can clearly see, including points of interest, interactable objects, and any other people who are present.}}", MessageType::Direction, false, 1);
		}
		pLLM->InstigateResponse(Responder::Narrator, MessageType::Narration, 1);
		break;
	case CommandType::Examine:
		if (!cmd.text.empty())
		{
			pLLM->PushMessage(Role::Narrator, "[{{user}} examines the " + cmd.text + ".]", MessageType::Narration, false, 1);
			pLLM->PushMessage(Role::Director, "{{Describe to {{user}} the " + cmd.text + " in minute detail.}}", MessageType::Direction, false, 1);
			pLLM->InstigateResponse(Responder::Narrator, MessageType::Narration, 1);
		}
		break;
	}
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

#if AUTOCHAT
void MainFrame::AutoChat()
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

	pLLM->SendMessage(message);
}
#endif

void MainFrame::PollStatus()
{
	auto pLLM = Application::GetLLM();
	if (pLLM)
	{
		auto status = pLLM->GetStatus();
		if (status.bInvalid)
		{
			UnloadModel();
			_pStatusBar->SetMessage("Error occurred. Model unloaded.");
			_pStatusBar->SetModelInfo("", 0, 0);
			return;
		}

		if (status.bReady)
			_pStatusBar->SetModelInfo(status.modelName, status.allocCtxSize, status.usedCtxSize);
	}
}

bool MainFrame::HandleKeyPress(SDL_Keycode key)
{
	auto pLLM = Application::GetLLM();
	switch (key)
	{
	case SDLK_F2:
		LoadModel();
		return true;
	case SDLK_F3:
		UnloadModel();
		return true;
	case SDLK_F9:
	{
		auto [responseId, subMessageId] = _pChatScroll->GetLastMessage();
		if (!pLLM->Continue(responseId, subMessageId))
			return pLLM->InstigateResponse(Responder::Bot, MessageType::Undefined);
		break;
	}
	case SDLK_F10:
		pLLM->Halt();
#if AUTOCHAT
		_bAutoChat = false;
#endif		
		break;

#if AUTOCHAT
	case SDLK_F5:
		_bAutoChat = !_bAutoChat;
		return true;
#endif
	}
	return false;
}