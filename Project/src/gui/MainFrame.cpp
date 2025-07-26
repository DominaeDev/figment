#include "gui/MainFrame.h"
#include "gui/Area.h"
#include "gui/Panel.h"
#include "gui/StaticText.h"
#include "gui/HorizontalSizer.h"
#include "gui/VerticalSizer.h"
#include "gui/TextBox.h"
#include "gui/Color.h"
#include "gui/ChatScroll.h"
#include "gui/ChatMessage.h"
#include "gui/StatusBar.h"
#include "gui/Renderers.h"
#include "gui/TextureStore.h"
#include "model/AppState.h"
#include "llm/LLMInstance.h"
#include "llm/LLMUtility.h"
#include "util/StringUtility.h"
#include "util/CommandParser.h"
#include "util/Common.h"
#include "Constants.h"
#include <format>
#include <ranges>

MainFrame* MainFrame::s_pInstance = nullptr;

MainFrame::MainFrame(SDL_Window* pWindow) : Frame(pWindow)
{
	SetForegroundColor(Colors::Black);
	SetBackgroundColor(Colors::AppBackground);

	auto mainArea = new Area(this);

	auto leftPanel = new Panel(mainArea);
	leftPanel->SetSize(200, -1);

	auto centerPanel = new Panel(mainArea);
	centerPanel->SetBackgroundColor(Colors::ChatBackground);
	centerPanel->SetSize(toF(Constants::ChatScrollWidth), -1);

	auto rightPanel = new Panel(mainArea);
	rightPanel->SetSize(200, -1);
	rightPanel->SetMinSize(200, -1);

	auto pStaticText = new StaticText(centerPanel, "", FontFace::Default, Constants::DefaultFontSize);
	pStaticText->SetAlignment(TextAlignment::Middle_Center);
	pStaticText->SetSize(80, 80);
	pStaticText->SetMinSize(-1, 80);
	pStaticText->SetBackgroundColor(Color { 255, 255, 0, SDL_ALPHA_OPAQUE });
	pStaticText->SetVisible(false);

	_pChatScroll = new ChatScroll(centerPanel);

	auto pTextBox = new TextBox(centerPanel, FontFace::Default, Constants::DefaultFontSize);
	pTextBox->SetSize(-1, 88);
	pTextBox->SelectAll();

	auto pCenterSizer = new VerticalSizer();
	pCenterSizer->Add(_pChatScroll, -1, Sizer::Expand | Sizer::Bottom, 8);
	pCenterSizer->Add(pTextBox, 0, Sizer::AlignBottom | Sizer::Expand);
	centerPanel->SetSizer(pCenterSizer);

	auto mainSizer = new HorizontalSizer();
	mainSizer->Add(leftPanel, -1, Sizer::Expand);
	mainSizer->Add(centerPanel, 0, Sizer::Expand | Sizer::Bottom, 24);
	mainSizer->Add(rightPanel, -1, Sizer::Expand);
	mainArea->SetSizer(mainSizer);

	// Status bar
	_pStatusBar = new StatusBar(this);

	auto topSizer = new VerticalSizer();
	topSizer->Add(mainArea, -1, Sizer::Expand);
	topSizer->Add(_pStatusBar, 0, Sizer::Expand);

	SetSizer(topSizer);
	
	pTextBox->SetEnterPressedCallback([this](string text) {
		OnCommand(CommandParser::Parse(text));
	});

	auto pTextBoxBG = new NineGridBackgroundRenderer();
	pTextBoxBG->SetCornerSize(10.0f);
	pTextBoxBG->SetColors(Colors::White, Color { 0xb9, 0xb2, 0x8f, 0xFF });
	pTextBoxBG->SetTextures(TextureStore::GetTexture(TextureType::TEXTBOX_BG), TextureStore::GetTexture(TextureType::TEXTBOX_BORDER));
	pTextBox->SetBackgroundRenderer(pTextBoxBG);

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

void MainFrame::OnRender(Renderer* pRenderer)
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
		std::shared_ptr<ChatSession> pSession = std::make_shared<ChatSession>();
		pSession->Initialize();
		pSession->LoadCharacter(Role::User, "./characters/user.xml");	//! @temp
		pSession->LoadCharacter(Role::Bot1, "./characters/bot1.xml");	//! @temp
		pSession->LoadCharacter(Role::Bot2, "./characters/bot2.xml");	//! @temp

		pLLM->InitializeChat(pSession, {});
		_pChatScroll->SetSession(pSession);

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

	auto fnRemovedMessageIds = [](std::vector<RemovedMessage> msgs) -> std::vector<string> {
		return to_vector(msgs | std::views::transform([](RemovedMessage msg) { return msg.responseId; }));
	};

	switch (cmd.type)
	{
	case CommandType::UserMessage:
#if _DEBUG
		if (!pLLM->IsReady())
		{
			static int turn = 0;
			auto [msgType, complete] = llm_util::detect_message_type(llm_util::process_message(cmd.text, ""));
			_pChatScroll->AddDummyMessage(turn % 2 == 0 ? "@USR" : "@BOT", turn % 2 == 0 ? Role::User : Role::Bot1, msgType, cmd.text);
			++turn;
		}
		else
#endif
		pLLM->SendMessage(cmd.text);
		break;
	case CommandType::SystemMessage:
		pLLM->PushMessage(Role::System, cmd.text, MessageType::SystemMessage);
		break;
	case CommandType::InstigateDialogue:
		pLLM->InstigateResponse(Responder::Bot, MessageType::Dialogue, 0);
		break;
	case CommandType::InstigateAction:
		pLLM->InstigateResponse(Responder::Bot, MessageType::Action, 0);
		break;
	case CommandType::PassTurn:
		pLLM->InstigateResponse(Responder::Bot, MessageType::Undefined, 3);
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
		_pChatScroll->RemoveMessages(fnRemovedMessageIds(removedIds));
		break;
	}
	case CommandType::RedoResponse:
	{
		auto removedIds = pLLM->RemoveMessages(1);
		if (!removedIds.empty())
		{
			Responder responder = Responder::Bot;
			if (removedIds.front().role == Role::Narrator)
				responder = Responder::Narrator;

			_pChatScroll->RemoveMessages(fnRemovedMessageIds(removedIds));
			pLLM->InstigateResponse(responder, MessageType::Undefined, 3);
		}
		break;
	}
	case CommandType::RollbackUserMessage:
	{
		auto removedIds = pLLM->RollbackUserMessage();
		_pChatScroll->RemoveMessages(fnRemovedMessageIds(removedIds));
		break;
	}
	case CommandType::Reset:
	{
		uint32_t seed = (uint32_t)atoi(cmd.text.c_str());
		if (!pLLM->IsReady() || pLLM->ResetChat(seed))
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
			pLLM->PushMessage(Role::Director, "{{Describe " + cmd.text + " from {{user}}'s perspective and pay attention to visual details.}}", MessageType::Direction, false, 1);
		}
		else 
		{
			pLLM->PushMessage(Role::Narrator, "[{{user}} takes a moment to observe their surroundings.]", MessageType::Narration, false, 1);
			pLLM->PushMessage(Role::Director, "{{Describe what {{user}} can clearly see, including points of interest, interactable objects, and anyone who are present.}}", MessageType::Direction, false, 1);
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
			text = pLLM->GetSession().ApplyNames(text);
			_autoScript = string_util::split(text, "\n");
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
		auto [status, ok] = pLLM->PollStatus();
		if (ok)
		{
			if (status.bInvalid)
				UnloadModel();
			_pStatusBar->SetModelInfo(status);
		}

		switch (status.signal)
		{
		case LLMStatusSignal::InitializingChat:
			SetStatusBar("Initializing chat...");
			break;
		case LLMStatusSignal::InitializedChat:
			SetStatusBar("Chat initialized");
			_pChatScroll->ClearMessages();
			break;
		case LLMStatusSignal::InitializeChatFailure:
			SetStatusBar("Failed to initialize chat");
			break;
		case LLMStatusSignal::LoadingModel:
			SetStatusBar("Loading model...");
			break;
		case LLMStatusSignal::LoadedModel:
			SetStatusBar("Model loaded");
			break;
		case LLMStatusSignal::LoadModelFailure:
			SetStatusBar("Failed to load model");
			break;
		case LLMStatusSignal::GenerationStarted:
			SetStatusBar("Generating response...");
			break;
		case LLMStatusSignal::GenerationComplete:
			SetStatusBar("Ready");
			break;
		default:
			break;
		}
	}
}

bool MainFrame::HandleKeyboardEvent(SDL_KeyboardEvent event)
{
	auto pLLM = Application::GetLLM();
	if (event.down && !event.repeat)
	{
		switch (event.key)
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
			if (!pLLM->Continue(responseId, subMessageId, true))
				return pLLM->InstigateResponse(Responder::Bot, MessageType::Undefined);
			break;
		}
		case SDLK_F10:
			pLLM->Halt();
#if AUTOCHAT
			_bAutoChat = false;
#endif		
			break;
		case SDLK_F11:
			pLLM->DumpContext((event.mod & SDL_KMOD_LSHIFT) != 0);
			break;
#if AUTOCHAT
		case SDLK_F5:
			_bAutoChat = !_bAutoChat;
			return true;
#endif
		}
	}
	return false;
}