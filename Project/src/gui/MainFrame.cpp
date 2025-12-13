#include "gui/MainFrame.h"
#include "gui/Area.h"
#include "gui/Panel.h"
#include "gui/StaticText.h"
#include "gui/HorizontalSizer.h"
#include "gui/VerticalSizer.h"
#include "gui/TextBox.h"
#include "gui/VariableList.h"
#include "gui/Color.h"
#include "gui/ChatScroll.h"
#include "gui/ChatMessage.h"
#include "gui/StatusBar.h"
#include "gui/Renderers.h"
#include "gui/TextureStore.h"
#include "model/AppState.h"
#include "model/ChatCommands.h"
#include "llm/LLMEngine.h"
#include "llm/LLMInstance.h"
#include "llm/LLMUtility.h"
#include "util/StringUtility.h"
#include "util/FileUtility.h"
#include "util/Common.h"
#include "Constants.h"
#include <format>
#include <ranges>
#include <cwctype>

using namespace common_util;
using namespace file_util;

MainFrame* MainFrame::s_pInstance = nullptr;

LLMOptions llmOptions = {
	LLMOption::GreetUser,
	LLMOption::Uncensored,
//	LLMOption::LimitMessages,
//	LLMOption::RandomizeMessageCount,
	LLMOption::StateVariables,
	LLMOption::ReportStateChanges,
//	LLMOption::Embeddings,
	LLMOption::UseMultipleSequences,
};

MainFrame::MainFrame(SDL_Window* pWindow) : Frame(pWindow)
{
	SetForegroundColor(Colors::Black);
	SetBackgroundColor(Colors::AppBackground);

	auto mainArea = new Area(this);

	auto leftPanel = new Panel(mainArea);
	leftPanel->SetSize(200, -1);

	auto centerPanel = new Panel(mainArea);
	centerPanel->SetBackgroundColor(Colors::ChatBackground);
	centerPanel->SetSize(toF(Constants::GUI::ChatScrollWidth), -1);

	auto rightPanel = new Panel(mainArea);
	rightPanel->SetSize(200, -1);
	rightPanel->SetMinSize(200, -1);

	auto pStaticText = new StaticText(centerPanel, "", FontFace::Default, Constants::GUI::DefaultFontSize);
	pStaticText->SetAlignment(TextAlignment::Middle_Center);
	pStaticText->SetSize(80, 80);
	pStaticText->SetMinSize(-1, 80);
	pStaticText->SetBackgroundColor(Color { 255, 255, 0, SDL_ALPHA_OPAQUE });
	pStaticText->SetVisible(false);

	_pChatScroll = new ChatScroll(centerPanel);

	_pTextBox = new TextBox(centerPanel, FontFace::Default, Constants::GUI::DefaultFontSize);
	_pTextBox->SetSize(-1, 88);
	_pTextBox->SelectAll();

	auto pCenterSizer = new VerticalSizer();
	pCenterSizer->Add(_pChatScroll, -1, Sizer::Expand | Sizer::Bottom, 8);
	pCenterSizer->Add(_pTextBox, 0, Sizer::AlignBottom | Sizer::Expand);
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
	
	_pTextBox->SetEnterPressedCallback([this](string text) {
		EnqueueCommand(ChatCommands::Parse(text));
	});

	auto pTextBoxBG = new NineGridBackgroundRenderer();
	pTextBoxBG->SetCornerSize(10.0f);
	pTextBoxBG->SetColors(Colors::White, Color { 0xb9, 0xb2, 0x8f, 0xFF });
	pTextBoxBG->SetTextures(TextureStore::GetTexture(TextureType::TEXTBOX_BG), TextureStore::GetTexture(TextureType::TEXTBOX_BORDER));
	_pTextBox->SetBackgroundRenderer(pTextBoxBG);

	_pTextBox->SetFocus(true);

	_pVariableList = new VariableList(mainArea);
	_pVariableList->SetPosition(10, 10);
	_pVariableList->SetVisible(false);

	std::map<string, string> test;
	test["Location"] = "Nice beach";
	test["Mood"] = "Terrible weather";
	_pVariableList->SetVariables(test);

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
		auto pLLM = Application::GetLLMInstance();
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

void MainFrame::InitializeModel()
{
	auto pLLMEngine = Application::GetLLMEngine();
	if (!pLLMEngine)
		return;

	_pStatus = pLLMEngine->GetStatusChannel();

	if (!pLLMEngine->IsInitialized())
	{
		SetStatusBar("Loading model...");
		pLLMEngine->Initialize(string(Constants::DefaultModelLocation), 
			llmOptions.IsSet(LLMOption::Embeddings) ? string(Constants::Embedding::DefaultModelLocation) : "",
			[this](int percent) 
			{
				SetStatusBar(std::format("Loading model... {0}%", percent));
			},
			[this, pLLMEngine](bool bSuccess)
			{
				if (bSuccess)
					Application::SetLLMInstance(pLLMEngine->CreateInstance());
			});
	}
}

void MainFrame::UnloadModel()
{
	auto pLLMEngine = Application::GetLLMEngine();
	if (pLLMEngine && pLLMEngine->IsInitialized())
	{
		pLLMEngine->Shutdown();
		SetStatusBar("Model unloaded");

		_pStatus.reset();
#if AUTOCHAT
		_bAutoChat = false;
#endif
	}
}

void MainFrame::StartChat()
{
	auto pLLM = Application::GetLLMInstance();
	if (pLLM && !pLLM->IsInitialized())
	{
		ChatSession session;
		session.Initialize(llmOptions);
		session.LoadCharacter(Role::User, "./characters/user.xml");	//! @temp
		session.LoadCharacter(Role::Bot1, "./characters/bot1.xml");	//! @temp
		session.LoadCharacter(Role::Bot2, "./characters/bot2.xml");	//! @temp

		LLMChatArguments llmArgs {
			/*session*/ session,
			/*messages*/ {},
			/*options*/ llmOptions,
		};
		pLLM->InitializeChat(llmArgs);
		_pChatScroll->SetSession(session);

		_bStartedChat = true;
	}
}

void MainFrame::SetStatusBar(string message)
{
	s_pInstance->_pStatusBar->SetMessage(message);
}

bool MainFrame::OnCommand(ParsedChatCommand cmd)
{
	if (cmd.command == ChatCommand::Invalid)
		return false;

	auto pLLM = Application::GetLLMInstance();
	if (!pLLM)
		return false;

	auto filter_msg_ids = [](std::vector<RemovedMessage> msgs) -> std::vector<string> {
		return msgs
			| std::views::transform([](RemovedMessage msg) { return msg.responseId; })
			| std::ranges::to<std::vector<string>>();
	};

	auto fnRoleFromName = [pLLM](string text) -> Role {
		if (string_util::empty_or_whitespace(text))
			return Role::Undefined;
		if (std::iswdigit(string_util::from_utf8(text)[0]))
			return bot_from_index(std::stoi(text) - 1);
		for (auto kvp : pLLM->GetSession().GetCharactersByRole())
		{
			if (string_util::begins_with(kvp.second.shortName, text, true))
				return kvp.first;
		}
		return pLLM->GetSession().GetRoleOf(text);
	};

	switch (cmd.command)
	{
	case ChatCommand::UserMessage:
#if _DEBUG
		if (!pLLM->IsReady())
		{
			static int turn = 0;
			auto [msgType, complete] = llm_util::detect_message_type(llm_util::process_message(cmd.text, ""));
			_pChatScroll->AddDummyMessage(turn % 2 == 0 ? "@USR" : "@BOT", turn % 2 == 0 ? Role::User : Role::Bot1, msgType, cmd.text);
			++turn;
			return true;
		}
#endif
		return pLLM->SendMessage(cmd.text);
	case ChatCommand::SystemMessage:
		return pLLM->PushMessage(Role::System, cmd.text, MessageType::SystemMessage);
	case ChatCommand::InstigateDialogue:
	{
		Role targetRole = fnRoleFromName(cmd.text);
		return pLLM->Instigate(targetRole, MessageType::Dialogue, 1);
	}
	case ChatCommand::InstigateAction:
	{
		Role targetRole = fnRoleFromName(cmd.text);
		return pLLM->Instigate(targetRole, MessageType::Action, 1);
	}
	case ChatCommand::PassTurn:
	{
		Role targetRole = fnRoleFromName(cmd.text);
		return pLLM->Instigate(targetRole, MessageType::Undefined, 3);
	}
	case ChatCommand::Impersonate:
		return pLLM->Instigate(Role::User, MessageType::Dialogue, 1);
	case ChatCommand::Narrate:
		if (cmd.text.empty())
			return pLLM->Instigate(Role::Narrator, MessageType::Narration, 1);
		else
			return pLLM->PushMessage(Role::Narrator, "[" + cmd.text + "]", MessageType::Narration);
	case ChatCommand::Instruct:
		if (!cmd.text.empty())
			return pLLM->Instruct(cmd.text);
	case ChatCommand::RemoveLast:
	{
		int n = atoi(cmd.text.c_str());
		auto removedMsgs = pLLM->RemoveMessages(std::max(n, 1));
		auto removedIds = filter_msg_ids(removedMsgs);
		return _pChatScroll->RemoveMessages(removedIds);
	}
	case ChatCommand::RedoResponse:
	{
		auto removedMsgs = pLLM->RemoveMessages(1);
		if (!removedMsgs.empty())
		{
			Role responder = removedMsgs.front().role;
			if (!is_bot(responder))
				responder = Role::Undefined;

			auto removedIds = filter_msg_ids(removedMsgs);
			_pChatScroll->RemoveMessages(removedIds);
			return pLLM->Instigate(responder, MessageType::Undefined, 3);
		}
		break;
	}
	case ChatCommand::RollbackUserMessage:
	{
		auto removedMsgs = pLLM->RollbackUserMessage();
		if (!removedMsgs.empty())
		{
			auto removedIds = filter_msg_ids(removedMsgs);
			_pChatScroll->RemoveMessages(removedIds);
			_pTextBox->SetText(removedMsgs[0].content);
		}
		break;
	}
	case ChatCommand::Reset:
	{
		uint32_t seed = (uint32_t)atoi(cmd.text.c_str());
		if (!pLLM->IsReady() || pLLM->ResetChat(seed))
			_pChatScroll->ClearMessages();
		queue_clear(_commandQueue);
		return true;
	}
	case ChatCommand::Reseed:
	{
		uint32_t seed = (uint32_t)atoi(cmd.text.c_str());
		if (seed == 0)
			seed = 0xFFFFFFFF;
		pLLM->Reseed(seed);
		break;
	}
	case ChatCommand::Look:
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
		return pLLM->Instigate(Role::Narrator, MessageType::Narration, 1);
	case ChatCommand::Examine:
		if (!cmd.text.empty())
		{
			pLLM->PushMessage(Role::Narrator, "[{{user}} examines the " + cmd.text + ".]", MessageType::Narration, false, 1);
			pLLM->PushMessage(Role::Director, "{{Describe what {{user}} is able to find, if anything, " + cmd.text + " in minute detail.}}", MessageType::Direction, false, 1);
			return pLLM->Instigate(Role::Narrator, MessageType::Narration, 1);
		}
		break;
	case ChatCommand::GenerateEmbedding:
#if _DEBUG
		return pLLM->GenerateEmbedding(cmd.text);
#else
		return false;
#endif
	case ChatCommand::NewStateVariable:
		if (!cmd.text.empty())
		{
			size_t pos_eq = cmd.text.find('=');
			if (pos_eq != string::npos)
			{
				string name = string_util::trim(cmd.text.substr(0, pos_eq));
				string value = string_util::trim(cmd.text.substr(pos_eq + 1));
				if (pLLM->SetStateVariable(name, value, true))
				{
					_pChatScroll->AddSystemMessage(std::format("{} = {}", name, value));
					return true;
				}
			}
		}
		return false;
	case ChatCommand::SetStateVariable:
		if (!cmd.text.empty())
		{
			size_t pos_eq = cmd.text.find('=');
			if (pos_eq != string::npos)
			{
				string name = string_util::trim(cmd.text.substr(0, pos_eq));
				string value = string_util::trim(cmd.text.substr(pos_eq + 1));
				if (pLLM->SetStateVariable(name, value, false))
				{
					_pChatScroll->AddSystemMessage(std::format("{} = {}", name, value));
					return true;
				}
			}
		}
		return false;
	}

	return false;
}

#if AUTOCHAT
void MainFrame::AutoChat()
{
	auto pLLMEngine = Application::GetLLMEngine();

	if (!pLLMEngine->IsInitialized())
	{
		if (!pLLMEngine->IsInitializing())
			InitializeModel();
		return;
	}

	auto pLLMInstance = Application::GetLLMInstance();
	if (!pLLMInstance || !pLLMInstance->IsReady() || pLLMInstance->IsGenerating())
		return;

	if (_autoScript.empty())
	{
		if (auto script = ReadTextFile("resources/auto_script.txt"))
		{
			string text = script.value();
			text = pLLMInstance->GetSession().ApplyNames(text);
			_autoScript = string_util::split(text, '\n');
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

	pLLMInstance->SendMessage(message);
}
#endif

void MainFrame::PollStatus()
{
	if (_pStatus)
	{
		auto pLLMInstance = Application::GetLLMInstance();
		auto status = _pStatus->PollStatus();
		if (status.IsInvalid())
			UnloadModel();
		_pStatusBar->SetModelInfo(status);

		switch (status.signal)
		{
		case LLMStatusSignal::InitializingChat:
			SetStatusBar("Initializing chat...");
			break;
		case LLMStatusSignal::InitializedChat:
			SetStatusBar("Chat initialized");
			_pChatScroll->ClearMessages();
			if (pLLMInstance)
			{
				_pVariableList->SetVariables(pLLMInstance->GetStateVariables());
				_pVariableList->SetVisible(true);
			}
			queue_clear(_commandQueue);
			break;
		case LLMStatusSignal::InitializeChatFailure:
			SetStatusBar("Failed to initialize chat");
			break;
		case LLMStatusSignal::LoadingModel:
			SetStatusBar("Loading model...");
			break;
		case LLMStatusSignal::LoadedModel:
			SetStatusBar("Model loaded");
			queue_clear(_commandQueue);
			StartChat();
			break;
		case LLMStatusSignal::UnloadedModel:
			SetStatusBar("Model unloaded");
			_pVariableList->SetVisible(false);
			Application::SetLLMInstance(nullptr);
			queue_clear(_commandQueue);
			break;
		case LLMStatusSignal::LoadModelFailure:
			SetStatusBar("Failed to load model");
			break;
		case LLMStatusSignal::GenerationStarted:
			SetStatusBar("Generating response...");
			break;
		case LLMStatusSignal::RebuildingContext:
			SetStatusBar("Rebuilding context...");
			break;
		case LLMStatusSignal::GenerationComplete:
			SetStatusBar("Ready");
			if (pLLMInstance)
				_pVariableList->SetVariables(pLLMInstance->GetStateVariables());
			NextQueuedCommand();
			break;
		default:
			break;
		}
	}
}

bool MainFrame::HandleKeyboardEvent(SDL_KeyboardEvent event)
{
	auto pLLM = Application::GetLLMInstance();

	bool bShiftDown = (event.mod & SDL_KMOD_SHIFT) != 0;
	bool bAltDown = (event.mod & SDL_KMOD_ALT) != 0;
	bool bCtrlDown = (event.mod & SDL_KMOD_CTRL) != 0;

	if (event.down && !event.repeat)
	{
		switch (event.key)
		{
		case SDLK_F2:
			InitializeModel();
			return true;
		case SDLK_F3:
			UnloadModel();
			return true;
		case SDLK_F9:
		{
			auto [responseId, subMessageId] = _pChatScroll->GetLastMessage();
			if (!pLLM->Continue(responseId, subMessageId, true))
				return pLLM->Instigate(Role::Undefined, MessageType::Undefined);
			break;
		}
		case SDLK_F10:
			pLLM->Halt();
			queue_clear(_commandQueue);
#if AUTOCHAT
			_bAutoChat = false;
#endif		
			break;
#if _DEBUG
		case SDLK_F11:
			if (pLLM->IsReady())
				pLLM->DumpContext();
			break;
#endif
#if AUTOCHAT
		case SDLK_F5:
			_bAutoChat = !_bAutoChat;
			return true;
#endif
		case SDLK_TAB:
			if (pLLM->IsInitialized())
			{
				_pVariableList->SetVariables(pLLM->GetStateVariables());
				_pVariableList->SetVisible(!_pVariableList->IsEmpty());
			}
			break;
		}
	}

	else if (!event.down && !event.repeat) // Release
	{
		switch (event.key)
		{
		case SDLK_TAB:
			_pVariableList->SetVisible(false);
			break;
		}
	}
	return false;
}

void MainFrame::EnqueueCommand(ParsedChatCommand cmd)
{
	auto pLLM = Application::GetLLMInstance();

	if (pLLM && pLLM->IsGenerating())
	{
		if (_commandQueue.size() < 3)
			_commandQueue.push(cmd);
	}
	else
		OnCommand(cmd);
}

void MainFrame::NextQueuedCommand()
{
	while (!_commandQueue.empty())
	{
		ParsedChatCommand command = _commandQueue.front();
		_commandQueue.pop();

		if (OnCommand(command))
			break;
	}
}

