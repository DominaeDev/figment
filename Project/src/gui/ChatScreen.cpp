#include <pch.h>
#include "gui/ChatScreen.h"

#include "gui/MainFrame.h"
#include "gui/GUICommon.h"
#include "gui/CustomRenderers.h"
#include "gui/ChatScroll.h"
#include "gui/ChatMessage.h"
#include "gui/StatusBar.h"
#include "gui/AppResources.h"
#include "gui/VariableList.h"
#include "model/AppState.h"
#include "model/ChatCommands.h"
#include "model/ChatCommandExecutor.h"
#include "model/UserManager.h"
#include "model/AssetManager.h"
#include "llm/LLMBackend.h"
#include "llm/LLMInstance.h"
#include "llm/LLMUtility.h"
#include "util/StringUtility.h"
#include "util/Common.h"
#include "fs/Serialization.h"
#include "fs/FileUtility.h"
#include "Constants.h"
#include <format>
#include <ranges>

using namespace fig::util;
using namespace fig::llm;
using namespace fig::io::data;
using namespace fig::io;

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

namespace fig::gui
{
	ChatScreen::ChatScreen(Frame* pParent) : Screen(pParent)
	{
		SetForegroundColor(Colors::Black);
		SetBackgroundColor(Colors::AppBackground);

		auto leftPanel = new Panel(this);
		leftPanel->SetSize(200, -1);

		auto centerPanel = new Panel(this);
		centerPanel->SetBackgroundColor(Colors::ChatBackground);
		centerPanel->SetSize(toF(Constants::GUI::ChatScrollWidth), -1);

		auto rightPanel = new Panel(this);
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
		SetSizer(mainSizer);

		_pTextBox->SetEnterPressedCallback([this](fig::string text) {
			EnqueueCommand(ChatCommands::Parse(text));
		});

		auto pTextBoxBG = new NineGridRenderer();
		pTextBoxBG->SetCornerSize(10.0f);
		pTextBoxBG->SetExtend(5.0f);
		pTextBoxBG->SetColor(Colors::White);
		pTextBoxBG->SetTexture(AppResources::GetTexture(TextureType::TEXTBOX_BG));
		_pTextBox->SetBackgroundRenderer(pTextBoxBG);
		auto pTextBoxBorder = new NineGridRenderer();
		pTextBoxBorder->SetCornerSize(10.0f);
		pTextBoxBorder->SetExtend(5.0f);
		pTextBoxBorder->SetColor(Color { 0xb9, 0xb2, 0x8f, 0xFF });
		pTextBoxBorder->SetTexture(AppResources::GetTexture(TextureType::TEXTBOX_BORDER));
		_pTextBox->SetBorderRenderer(pTextBoxBorder);

		_pTextBox->SetFocus(true);

		_pVariableList = new VariableList(this);
		_pVariableList->SetPosition(10, 10);
		_pVariableList->SetVisible(false);

		std::map<fig::string, fig::string> test;
		test["Location"] = "Nice beach";
		test["Mood"] = "Terrible weather";
		_pVariableList->SetVariables(test);

		InvalidateLayout();
	}

	void ChatScreen::OnUpdate(float fElapsed)
	{
		if (_bStartedChat)
		{
			_bStartedChat = false;
			if (DefaultChatOptions.flags.IsSet(ChatOptions::Flag::GreetUser))
			{
				auto pLLM = Global::GetLLMInstance();
				if (pLLM)
					pLLM->GreetUser();
			}
		}

		// Poll llm status
		_fPollingCounter += fElapsed;
		if (_fPollingCounter > 0.1f)
			PollStatus();

#if ENABLE_AUTO_CHAT
		if (_bAutoChat) AutoChat();
#endif
	}

	void ChatScreen::OnRender(Renderer* pRenderer)
	{
		DrawBackground(pRenderer);
	}

	void ChatScreen::InitializeModel()
	{
		auto& engine = Global::GetLLMEngine();

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
				{
					auto pInstance = engine.CreateInstance(Constants::Context::DefaultSize, DefaultChatOptions.flags.IsSet(ChatOptions::Flag::Embeddings));
					Global::SetLLMInstance(pInstance);
				}
			});
		}
	}

	void ChatScreen::UnloadModel()
	{
		auto& engine = Global::GetLLMEngine();
		if (engine.IsInitialized())
		{
			engine.Shutdown();
			SetStatusBar(fig::strings::Status::ModelUnloaded);

#if ENABLE_AUTO_CHAT
			_bAutoChat = false;
#endif
		}
	}

	void ChatScreen::StartChat()
	{
		auto pLLM = Global::GetLLMInstance();
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

	void ChatScreen::SetStatusBar(fig::string_view message)
	{
		MainFrame::GetInstance().SetStatusBar(toStr(message));
	}

	bool ChatScreen::OnCommand(ParsedChatCommand cmd)
	{
		return ChatCommandExecutor::Execute(cmd,
			ChatCommandExecutor::Context {
				.pLLM = Global::GetLLMInstance(),
				.pChatFrame = this,
			});
	}

#if ENABLE_AUTO_CHAT
	void ChatScreen::AutoChat()
	{
		auto& engine = Global::GetLLMEngine();
		static std::mt19937 rng {};
		static std::uniform_int_distribution<int> dist(0, 99);
		if (!engine.IsInitialized())
		{
			if (!engine.IsInitializing())
				InitializeModel();
			return;
		}

		auto pLLMInstance = Global::GetLLMInstance();
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

	void ChatScreen::PollStatus()
	{
		auto pChannel = Global::GetLLMEngine().GetStatusChannel();
		if (!pChannel)
			return;

		if (auto status = pChannel->PollStatus())
		{
			auto pLLMInstance = Global::GetLLMInstance();
			MainFrame::GetInstance().SetStatusBar(status.value());

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
				Global::SetLLMInstance(nullptr);
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

	bool ChatScreen::OnKeyboardEvent(KeyboardEvent& event)
	{
		bool bModNone = event.modifiers == KeyModifiers::None;

		if (event.pressed)
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
				if (event.modifiers == KeyModifiers { KeyModifier::Control })
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
		else // Release
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
		auto pLLM = Global::GetLLMInstance();
		if (pLLM)
		{
			if (event.pressed)
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

	void ChatScreen::EnqueueCommand(ParsedChatCommand cmd)
	{
		auto pLLM = Global::GetLLMInstance();

		if (pLLM && pLLM->IsGenerating())
		{
			if (_commandQueue.size() < 3)
				_commandQueue.push(cmd);
		}
		else
			OnCommand(cmd);
	}

	void ChatScreen::NextQueuedCommand()
	{
		while (!_commandQueue.empty())
		{
			ParsedChatCommand command = _commandQueue.front();
			_commandQueue.pop();

			if (OnCommand(command))
				break;
		}
	}

	void ChatScreen::Close()
	{
		SDL_Event quit_event;
		SDL_zero(quit_event);  /* SDL will copy this entire struct! Initialize to keep memory checkers happy. */
		quit_event.type = SDL_EVENT_QUIT;
		SDL_PushEvent(&quit_event);
	}
}