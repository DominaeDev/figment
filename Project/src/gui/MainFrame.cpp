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
#include "gui/CustomRenderers.h"
#include "gui/TextureStore.h"
#include "model/AppState.h"
#include "model/ChatCommands.h"
#include "model/ChatCommandExecutor.h"
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

fig::gui::MainFrame* MainFrame::s_pInstance = nullptr;

constexpr ChatOptions DefaultChatOptions {
	.flags = {
		ChatOptions::Flag::GreetUser,
		ChatOptions::Flag::Uncensored,
	//	ChatOptions::Flag::LimitMessages,
	//	ChatOptions::Flag::RandomizeMessageCount,
		ChatOptions::Flag::StateVariables,
		ChatOptions::Flag::ReportStateChanges,
	//	ChatOptions::Flag::Embeddings,
	},
	.multiBotMode = ChatOptions::MultiBotMode::MultipleSequences,
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
}

MainFrame::~MainFrame()
{
}

void MainFrame::OnUpdate(float fDeltaTime)
{
	if (_bStartedChat)
	{
		_bStartedChat = false;
		auto pLLM = ApplicationState::GetLLMInstance();
		if (pLLM)
			pLLM->GreetUser();
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
		SetStatusBar(GlobalStrings::Status::LoadingModel);

		engine.Initialize(fig::string(Constants::DefaultModelLocation), 
			DefaultChatOptions.flags.IsSet(ChatOptions::Flag::Embeddings) ? toStr(Constants::Embedding::DefaultModelLocation) : "",
			[this](int percent) 
			{
				SetStatusBar(std::format(GlobalStrings::Status::LoadingModelPercentFmt, percent));
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
		SetStatusBar(GlobalStrings::Status::ModelUnloaded);

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
	}

	if (_autoScript.empty())
	{
		_bAutoChat = false;
		return;
	}

	fig::string message = _autoScript[_autoScriptIndex];
	_autoScriptIndex = ++_autoScriptIndex % _autoScript.size();

	pLLMInstance->SendMessage(message);
}
#endif

void MainFrame::PollStatus()
{
	auto pStatus = ApplicationState::GetLLMEngine().GetStatusChannel();
	if (pStatus)
	{
		auto pLLMInstance = ApplicationState::GetLLMInstance();
		auto status = pStatus->PollStatus();
		_pStatusBar->SetModelInfo(status);

		switch (status.signal)
		{
		case LLMStatusSignal::ChatInitializing:
			SetStatusBar(GlobalStrings::Status::InitializingChat);
			break;
		case LLMStatusSignal::ChatInitialized:
			SetStatusBar(GlobalStrings::Status::ChatInitialized);
			_pChatScroll->ClearMessages();
			if (pLLMInstance)
			{
				_pVariableList->SetVariables(pLLMInstance->GetStateVariables());
				_pVariableList->SetVisible(true);
			}
			queue_clear(_commandQueue);
			break;
		case LLMStatusSignal::ChatInitializationFailure:
			SetStatusBar(GlobalStrings::Status::FailedToInitializeChat);
			break;
		case LLMStatusSignal::ModelLoading:
			SetStatusBar(GlobalStrings::Status::LoadingModel);
			break;
		case LLMStatusSignal::ModelLoaded:
			SetStatusBar(GlobalStrings::Status::ModelLoaded);
			queue_clear(_commandQueue);
			StartChat();
			break;
		case LLMStatusSignal::ModelUnloaded:
			SetStatusBar(GlobalStrings::Status::ModelUnloaded);
			_pVariableList->SetVisible(false);
			ApplicationState::SetLLMInstance(nullptr);
			queue_clear(_commandQueue);
			break;
		case LLMStatusSignal::ModelLoadFailure:
			SetStatusBar(GlobalStrings::Status::FailedToLoadModel);
			break;
		case LLMStatusSignal::ModelUnloadRequest:
			UnloadModel();
			break;
		case LLMStatusSignal::GenerationStarted:
			SetStatusBar(GlobalStrings::Status::GeneratingResponse);
			break;
		case LLMStatusSignal::RebuildingKVCache:
			SetStatusBar(GlobalStrings::Status::RebuildingContext);
			break;
		case LLMStatusSignal::GenerationComplete:
			SetStatusBar(GlobalStrings::Status::Ready);
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
	auto pLLM = ApplicationState::GetLLMInstance();

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
#if ENABLE_AUTO_CHAT
			_bAutoChat = false;
#endif		
			break;
#if _DEBUG
		case SDLK_F11:
			if (pLLM->IsReady())
				pLLM->DumpContext();
			break;

		case SDLK_F12:
			if (bCtrlDown)
				Close();
			break;
#endif
#if ENABLE_AUTO_CHAT
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