#include <pch.h>
#include "gui/MainFrame.h"

#include "gui/GUICommon.h"
#include "gui/CustomRenderers.h"
#include "gui/ChatScroll.h"
#include "gui/ChatMessage.h"
#include "gui/StatusBar.h"
#include "gui/TextureStore.h"
#include "gui/VariableList.h"
#include "model/AppState.h"
#include "model/ChatCommands.h"
#include "model/ChatCommandExecutor.h"
#include "model/UserManager.h"
#include "llm/LLMEngine.h"
#include "llm/LLMInstance.h"
#include "llm/LLMUtility.h"
#include "util/StringUtility.h"
#include "util/FileUtility.h"
#include "util/Common.h"
#include "Constants.h"
#include <format>
#include <ranges>

using namespace fig::gui;
using namespace fig::common_util;
using namespace fig::file_util;
using namespace fig::string_util;
using namespace fig::llm;
using namespace fig::data;
using namespace fig::fs;

fig::gui::MainFrame* MainFrame::s_pInstance = nullptr;

template<typename T>
void queue_clear(std::queue<T>& q)
{
	std::queue<T> empty;
	std::swap(q, empty);
}

constexpr ChatOptions DefaultChatOptions {
	.flags = {
		ChatOptions::Flag::GreetUser,
		ChatOptions::Flag::Uncensored,
//		ChatOptions::Flag::LimitMessages,
//		ChatOptions::Flag::RandomizeMessageCount,
//		ChatOptions::Flag::StateVariables,
//		ChatOptions::Flag::ReportStateChanges,
//		ChatOptions::Flag::Embeddings,
	},
	.groupChatMode = ChatOptions::GroupChatMode::SwapSequences,
};

MainFrame::MainFrame(Window* pWindow) : Frame(pWindow)
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

	_pTextBox = new TextBox(centerPanel, FontFace::Default, Constants::GUI::DefaultFontSize, { TextBox::Flag::Multi, TextBox::Flag::Autosize });
	_pTextBox->SetSize(-1, 88);
	_pTextBox->SetMinRows(2);
	_pTextBox->SetMaxRows(8);

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
	
	_pTextBox->SetEnterPressedCallback([this](fig::string text) {
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

	std::map<fig::string, fig::string> test;
	test["Location"] = "Nice beach";
	test["Mood"] = "Terrible weather";
	_pVariableList->SetVariables(test);

	InvalidateLayout();
	s_pInstance = this;

	//! @temp
	UserManager userMngr {};
	userMngr.CreateDefaultProfile();

	userMngr.SignIn("Default profile", "");
}

MainFrame::~MainFrame()
{
}

void MainFrame::OnUpdate(float fDeltaTime)
{
	if (_bStartedChat)
	{
		_bStartedChat = false;
		if (DefaultChatOptions.flags.IsSet(ChatOptions::Flag::GreetUser))
		{
			auto pLLM = ApplicationState::GetLLMInstance();
			if (pLLM)
				pLLM->GreetUser();
		}
	}

	// Poll llm status
	_fPollingCounter += fDeltaTime;
	if (_fPollingCounter > 0.1f)
		PollStatus();

#if ENABLE_AUTO_CHAT
	if (_bAutoChat) AutoChat();
#endif
}

void MainFrame::OnRender(Renderer* pRenderer)
{
	DrawBackground(pRenderer);
}

void MainFrame::InitializeModel()
{
	auto& engine = ApplicationState::GetLLMEngine();

	if (!engine.IsInitialized())
	{
		SetStatusBar(fig::strings::Status::LoadingModel);

		engine.Initialize(fig::string(Constants::DefaultModelLocation), 
			DefaultChatOptions.flags.IsSet(ChatOptions::Flag::Embeddings) ? toStr(Constants::Embedding::DefaultModelLocation) : "",
			[this](int percent) 
			{
				SetStatusBar(std::format(fig::strings::Status::LoadingModelPercentFmt, percent));
			},
			[this, &engine](bool bSuccess)
			{
				if (bSuccess)
					ApplicationState::SetLLMInstance(engine.CreateInstance(Constants::Context::DefaultSize, DefaultChatOptions.flags.IsSet(ChatOptions::Flag::Embeddings)));
			});
	}
}

void MainFrame::UnloadModel()
{
	auto& engine = ApplicationState::GetLLMEngine();
	if (engine.IsInitialized())
	{
		engine.Shutdown();
		SetStatusBar(fig::strings::Status::ModelUnloaded);

#if ENABLE_AUTO_CHAT
		_bAutoChat = false;
#endif
	}
}

void MainFrame::StartChat()
{
	auto pLLM = ApplicationState::GetLLMInstance();
	if (pLLM && !pLLM->IsInitialized())
	{
		ChatSession session;
		session.Initialize(DefaultChatOptions);
		session.LoadCharacter(Role::User, "./characters/user.xml");	//! @temp
		session.LoadCharacter(Role::Bot1, "./characters/bot1.xml");	//! @temp
		session.LoadCharacter(Role::Bot2, "./characters/bot2.xml");	//! @temp

		LLMChatArguments llmArgs {
			/*session*/ session,
			/*messages*/ {},
			/*options*/ DefaultChatOptions,
		};
		pLLM->Initialize(llmArgs);
		_pChatScroll->SetSession(session);

		_bStartedChat = true;
	}
	else
	{
		LogLn("Failed to initialize chat");
	}
}

void MainFrame::SetStatusBar(fig::string_view message)
{
	s_pInstance->_pStatusBar->SetMessage(toStr(message));
}

bool MainFrame::OnCommand(ParsedChatCommand cmd)
{
	return ChatCommandExecutor::Execute(cmd,
		ChatCommandExecutor::Context {
			.pLLM = ApplicationState::GetLLMInstance(),
			.pMainFrame = this,
		});
}

#if ENABLE_AUTO_CHAT
void MainFrame::AutoChat()
{
	auto& engine = ApplicationState::GetLLMEngine();
	static std::mt19937 rng {};
	static std::uniform_int_distribution<int> dist(0, 99);
	if (!engine.IsInitialized())
	{
		if (!engine.IsInitializing())
			InitializeModel();
		return;
	}

	auto pLLMInstance = ApplicationState::GetLLMInstance();
	if (!pLLMInstance || !pLLMInstance->IsReady() || pLLMInstance->IsGenerating())
		return;

	if (_autoScript.empty())
	{
		if (auto script = ReadTextFile("resources/auto_script.txt"))
		{
			fig::string text = script.value();
			text = pLLMInstance->GetSession().ApplyNames(text);
			_autoScript = split(text, '\n');
		}
		_autoScriptIndex = 0;
		rng.seed(Constants::LLM::DebugSeed);
	}

	if (_autoScript.empty())
	{
		_bAutoChat = false;
		return;
	}

	if (_bStartedChat)
		return; // Wait for greeting

	string command;
	int roll = dist(rng);
	if (roll < 5)
		command = "/erase";
	else if (roll < 10)
		command = "/redo";
	else if (roll < 15)
		command = "/instruct They think for a moment.";
	else
	{
		command = _autoScript[_autoScriptIndex];
		_autoScriptIndex = ++_autoScriptIndex % _autoScript.size();
	}

	LogLn(std::format(">> Auto: {}", command));
	EnqueueCommand(ChatCommands::Parse(command));
}
#endif

void MainFrame::PollStatus()
{
	auto pChannel = ApplicationState::GetLLMEngine().GetStatusChannel();
	if (!pChannel)
		return;

	if (auto status = pChannel->PollStatus())
	{
		auto pLLMInstance = ApplicationState::GetLLMInstance();
		_pStatusBar->SetModelInfo(status.value());

		switch (status.value().signal)
		{
		case LLMStatusSignal::ChatInitializing:
			SetStatusBar(fig::strings::Status::InitializingChat);
			break;
		case LLMStatusSignal::ChatInitialized:
			SetStatusBar(fig::strings::Status::ChatInitialized);
			_pChatScroll->ClearMessages();
			if (pLLMInstance)
			{
				_pVariableList->SetVariables(pLLMInstance->GetStateVariables());
				_pVariableList->SetVisible(true);
			}
			queue_clear(_commandQueue);
			break;
		case LLMStatusSignal::ChatInitializationFailure:
			SetStatusBar(fig::strings::Status::FailedToInitializeChat);
			break;
		case LLMStatusSignal::ModelLoading:
			SetStatusBar(fig::strings::Status::LoadingModel);
			break;
		case LLMStatusSignal::ModelLoaded:
			SetStatusBar(fig::strings::Status::ModelLoaded);
			queue_clear(_commandQueue);
			StartChat();
			break;
		case LLMStatusSignal::ModelUnloaded:
			SetStatusBar(fig::strings::Status::ModelUnloaded);
			_pVariableList->SetVisible(false);
			ApplicationState::SetLLMInstance(nullptr);
			queue_clear(_commandQueue);
			break;
		case LLMStatusSignal::ModelLoadFailure:
			SetStatusBar(fig::strings::Status::FailedToLoadModel);
			break;
		case LLMStatusSignal::ModelUnloadRequest:
			UnloadModel();
			break;
		case LLMStatusSignal::GenerationStarted:
			SetStatusBar(fig::strings::Status::GeneratingResponse);
			break;
		case LLMStatusSignal::RebuildingKVCache:
			SetStatusBar(fig::strings::Status::RebuildingContext);
			break;
		case LLMStatusSignal::GenerationComplete:
			SetStatusBar(fig::strings::Status::Ready);
			if (pLLMInstance)
				_pVariableList->SetVariables(pLLMInstance->GetStateVariables());
			NextQueuedCommand();
			break;
		default:
			break;
		}
	}
}

bool MainFrame::OnKeyboardEvent(SDL_KeyboardEvent& event)
{
	bool bCtrlDown = (event.mod & SDL_KMOD_CTRL) != 0;
	bool bShiftDown = (event.mod & SDL_KMOD_SHIFT) != 0;
	bool bAltDown = (event.mod & SDL_KMOD_ALT) != 0;

	bool bModNone = !bCtrlDown and !bShiftDown and !bAltDown;
	bool bModCtrl = bCtrlDown and !bShiftDown and !bAltDown;

	if (event.down && !event.repeat)
	{
		switch (event.key)
		{
		case SDLK_F2:
			if (bModNone)
			{
				InitializeModel();
				return true;
			}
			break;
		case SDLK_F3:
			if (bModNone)
			{
				UnloadModel();
				return true;
			}
			break;
#if _DEBUG
		case SDLK_F12:
			if (bModCtrl)
			{
				Close();
				return true;
			}
			break;
#endif
#if ENABLE_AUTO_CHAT
		case SDLK_F5:
			if (bModNone)
			{
				_bAutoChat = !_bAutoChat;
				return true;
			}
			break;
#endif
		}
	}
	else if (!event.down && !event.repeat) // Release
	{
		switch (event.key)
		{
		case SDLK_TAB:
			if (bModNone)
			{
				_pVariableList->SetVisible(false);
				return true;
			}
			break;
		}
	}

	// LLM shortcuts
	auto pLLM = ApplicationState::GetLLMInstance();
	if (pLLM)
	{
		if (event.down && !event.repeat)
		{
			switch (event.key)
			{
			case SDLK_F9:
				if (bModNone)
				{
					auto [responseId, subMessageId] = _pChatScroll->GetLastMessage();
					if (!pLLM->Continue(responseId, subMessageId, true))
						pLLM->Instigate(Role::Undefined, MessageType::Undefined); // Can't continue: Pass
					return true;
				}
				break;
			case SDLK_F10:
				if (bModNone)
				{
					pLLM->Halt();
					queue_clear(_commandQueue);
#if ENABLE_AUTO_CHAT
					_bAutoChat = false;
#endif		
					return true;
				}
				break;
#if _DEBUG
			case SDLK_F11:
				if (bModNone)
				{
					if (pLLM->IsReady())
						pLLM->DumpContext();
					return true;
				}
				break;
#endif
			case SDLK_TAB:
				if (bModNone)
				{
					if (pLLM->IsInitialized())
					{
						_pVariableList->SetVariables(pLLM->GetStateVariables());
						_pVariableList->SetVisible(!_pVariableList->IsEmpty());
					}
					return true;
				}
				break;
			}
		}
	}
	return false;
}

void MainFrame::EnqueueCommand(ParsedChatCommand cmd)
{
	auto pLLM = ApplicationState::GetLLMInstance();

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

void MainFrame::Close()
{
	SDL_Event quit_event;
	SDL_zero(quit_event);  /* SDL will copy this entire struct! Initialize to keep memory checkers happy. */
	quit_event.type = SDL_EVENT_QUIT;
	SDL_PushEvent(&quit_event);
}