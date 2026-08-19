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
#include "gui/InfoPanel.h"
#include "gui/ChatBackground.h"
#include "gui/Slider.h"
#include "gui/BehindChat.h"
#include "app/AppState.h"
#include "chat/ChatSession.h"
#include "chat/ChatCommands.h"
#include "chat/ChatCommandExecutor.h"
#include "user/UserManager.h"
#include "io/AssetManager.h"
#include "io/Serialization.h"
#include "io/FileUtility.h"
#include "data/ChatInstance.h"
#include "llm/LLMBackend.h"
#include "llm/LLMInstance.h"
#include "llm/LLMUtility.h"
#include "text/TextEvaluator.h"

using namespace fig::llm;
using namespace fig::data;
using namespace fig::chat;
using namespace fig::io;

namespace fig::gui
{
	ChatScreen::ChatScreen(Frame* pParent) : Screen(pParent)
	{
		_pBackground = CreateControl<ChatBackground>();
		_pBackground->SetBrightness(0.85f);
//		_pBackground->SetAlpha(0.85f);
//		_pBackground->SetSaturation(0.5f);
		_pBackground->SetBlur(3.0f);

		auto centerArea = _pBackground->CreateControl<Area>();
		centerArea->SetSize(Constants::GUI::ChatScrollWidth, -1);

		_pBehindChat = centerArea->CreateControl<BehindChat>();
		_pBehindChat->SetWidth(Constants::GUI::ChatScrollWidth + 200);
		_pBehindChat->SetColor(Color::Black.WithAlpha(0.45f));

		_pInfoPanel = CreateControl<InfoPanel>();
		_pChatScroll = centerArea->CreateControl<ChatScroll>();

		_pTextBox = centerArea->CreateControl<TextBox>(FontFace::Default, Constants::GUI::ChatBoxFontSize, TextBox::Flags { TextBox::Flag::Multi, TextBox::Flag::Autosize });
		_pTextBox->SetSize(Constants::GUI::ChatTextBoxWidth, 88);
		_pTextBox->SetMinRows(2);
		_pTextBox->SetMaxRows(8);

		auto pCenterSizer = centerArea->SetSizer<VerticalSizer>();
		pCenterSizer->Add(_pChatScroll, -1, SizerFlag::Fill | SizerFlag::Bottom, 8);
		pCenterSizer->Add(_pTextBox, 0, SizerFlag::AlignBottom | SizerFlag::AlignCenterHorizontal);

		auto mainSizer = SetSizer<HorizontalSizer>();
		mainSizer->Add(_pBackground, -1, SizerFlag::Fill);
		mainSizer->Add(_pInfoPanel, 0, SizerFlag::Expand);

		auto bgSizer = _pBackground->SetSizer<HorizontalSizer>();
		bgSizer->AddStretchSpacer();
		bgSizer->Add(centerArea, 0, SizerFlag::Fill | SizerFlag::Bottom, 24);
		bgSizer->AddStretchSpacer();

		_pTextBox->SetEnterPressedCallback([this](fig::string text) {
			EnqueueCommand(ChatCommands::Parse(text));
			_pTextBox->Clear();
		});

		auto pTextBoxBG = _pTextBox->SetBackgroundRenderer<TexturedBorderRenderer>(Resource::TEXTBOX_BG);
		pTextBoxBG->SetCornerScale(0.5f);
		pTextBoxBG->SetExtend(7.0f);
		pTextBoxBG->SetColor(Color::White);

		auto pTextBoxBorder = _pTextBox->SetBorderRenderer<TexturedBorderRenderer>(Resource::TEXTBOX_BORDER);
		pTextBoxBorder->SetCornerScale(0.5f);
		pTextBoxBorder->SetExtend(7.0f);
		pTextBoxBorder->SetColor(fig::color { 0xb9, 0xb2, 0x8f, 0xFF });

		if constexpr (Disabled)
		{
			_pVariableList = CreateControl<VariableList>();
			_pVariableList->SetPosition(10, 10);
			_pVariableList->SetVisible(false);

			std::map<fig::string, fig::string> test;
			test["Location"] = "Nice beach";
			test["Mood"] = "Terrible weather";
			_pVariableList->SetVariables(test);
		}
		
		if constexpr (Enabled)
		{
			auto pAlphaSlider = CreateControl<Slider>();
			pAlphaSlider->SetValue(1.0f);
			pAlphaSlider->SetPosition(8, 8 + 20 * 0);
			pAlphaSlider->SetDelegate([&](float value) {
				_pBackground->SetAlpha(value);
			});

			auto pBrightnessSlider = CreateControl<Slider>();
			pBrightnessSlider->SetValue(0.85f);
			pBrightnessSlider->SetPosition(8, 8 + 20 * 1);
			pBrightnessSlider->SetDelegate([&](float value) {
				_pBackground->SetBrightness(value);
			});

			auto pSaturationSlider = CreateControl<Slider>();
			pSaturationSlider->SetValue(1.0f);
			pSaturationSlider->SetPosition(8, 8 + 20 * 2);
			pSaturationSlider->SetDelegate([&](float value) {
				_pBackground->SetSaturation(value);
			});

			auto pBlurSlider = CreateControl<Slider>();
			pBlurSlider->SetValue(0.3f);
			pBlurSlider->SetPosition(8, 8 + 20 * 3);
			pBlurSlider->SetDelegate([&](float value) {
				int32_t blur = toI(value * 10);
				if (blur > 0)
					_pBackground->SetBlur(toF(blur + 2));
				else
					_pBackground->SetBlur(0.0f);
			});

			auto pUnderSlider = CreateControl<Slider>();
			pUnderSlider->SetValue(0.45f);
			pUnderSlider->SetPosition(8, 8 + 20 * 4);
			pUnderSlider->SetDelegate([this](float value) {
				auto color = Color::Black.WithAlpha(value);
				_pBehindChat->SetColor(color);
			});

			auto pChatSlider = CreateControl<Slider>();
			pChatSlider->SetValue(1.0f);
			pChatSlider->SetPosition(8, 8 + 20 * 5);
			pChatSlider->SetDelegate([this](float value) {
				_pChatScroll->SetAlpha(std::lerp(0.65f, 1.0f, value));
			});
		}

		InvalidateLayout();
	}

	void ChatScreen::OnUpdate(float fElapsed)
	{
		if (_bStartedChat)
		{
			_bStartedChat = false;
			if (Constants::LLM::DefaultChatOptions.flags.IsSet(ChatOptions::Flag::GreetUser))
			{
				auto pLLM = Global::GetLLMInstance();
				if (pLLM)
					pLLM->GreetUser();
			}
		}



#if ENABLE_AUTO_CHAT
		if (_bAutoChat) 
			AutoChat();
#endif
	}

	void ChatScreen::OnRender(fig::renderer_ptr pRenderer)
	{
		DrawBackground(pRenderer);
	}

	void ChatScreen::StartChat(const fig::chat::ChatStaging& staging, fig::uuid chatInstanceId, fig::uuid chatLogId)
	{
		auto pLLM = Global::GetLLMInstance();
		if (pLLM && !pLLM->IsInitialized())
		{
			_pSession = std::make_shared<fig::chat::ChatSession>();
			_pSession->Initialize(staging, Constants::LLM::DefaultChatOptions, chatInstanceId, chatLogId);

			LLMChatArguments llmArgs {
				.wpSession = _pSession,
				.options = Constants::LLM::DefaultChatOptions,
			};
			pLLM->Initialize(llmArgs);
			_pChatScroll->SetSession(_pSession);

			// Set portrait
			_pInfoPanel->SetSession(*_pSession);
			
			if (auto try_portrait = Global::GetUserContent().GetLargePortraitForCharacter(_pSession->GetCharacterIdOf(Role::Bot1))) //! @temp
				_pBackground->SetImage((*try_portrait).id);

			_pSession->Save();
			_bStartedChat = true;
			queue_clear(_commandQueue);
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
		auto& backend = Global::GetLLMBackend();
		static std::mt19937 rng {};
		static std::uniform_int_distribution<int> dist(0, 99);
		if (!backend.IsInitialized())
		{
			if (!backend.IsInitializing())
				MainFrame::GetInstance().InitializeModel();
			return;
		}

		auto pLLMInstance = Global::GetLLMInstance();
		if (!pLLMInstance || !pLLMInstance->IsReady() || pLLMInstance->IsGenerating())
			return;

		if (_autoScript.empty())
		{
			if (auto script = fig::io::ReadTextFile("resources/auto_script.txt"))
			{
				if (auto pSession = pLLMInstance->GetSession().lock())
				{
					fig::string text = eval_text(script.value(), pSession->GetContext());
					_autoScript = split(text, '\n');
				}
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

	bool ChatScreen::OnKeyboardEvent(KeyboardEvent& event)
	{
		if (event.pressed)
		{
#if ENABLE_AUTO_CHAT
			if (event.key == SDLK_F5 && event.modifiers.None)
			{
				_bAutoChat = !_bAutoChat;
				return true;
			}
#endif
		}
		else // Release
		{
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
					if (event.modifiers.None)
					{
						auto [responseId, subMessageId] = _pChatScroll->GetLastMessage();
						if (!pLLM->Continue(responseId, subMessageId, true))
							pLLM->Instigate(Role::Undefined, MessageType::Undefined); // Can't continue: Pass
						return true;
					}
					break;
				case SDLK_F10:
					if (event.modifiers.None)
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
					if (event.modifiers.None)
					{
						if (pLLM->IsReady())
							pLLM->DumpContext();
						return true;
					}
					break;
#endif
				case SDLK_TAB:
					if (event.modifiers.None)
					{
						if (pLLM->IsInitialized() and _pVariableList)
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

	EventResult ChatScreen::OnEvent(fig::event& event)
	{
		if (IsUserEvent(event, UserEvent::LLMChatInitializing))
		{
//			_pChatScroll->ClearMessages(); //! @todo: Uncomment when chat initialization is asynchronous
		}
		if (IsUserEvent(event, UserEvent::LLMChatInitialized))
		{
			auto pLLMInstance = Global::GetLLMInstance();
			if (pLLMInstance and _pVariableList)
			{
				_pVariableList->SetVariables(pLLMInstance->GetStateVariables());
				_pVariableList->SetVisible(true);
			}
			queue_clear(_commandQueue);
		}
		else if (IsUserEvent(event, UserEvent::LLMChatUnloaded))
		{
			_pSession.reset();
		}
		else if (IsUserEvent(event, UserEvent::LLMModelLoaded))
		{
//			StartChat();
		}
		else if (IsUserEvent(event, UserEvent::LLMModelUnloaded))
		{
			_pSession.reset();
			if (_pVariableList)
				_pVariableList->SetVisible(false);
			Global::SetLLMInstance(nullptr);
			queue_clear(_commandQueue);
#if ENABLE_AUTO_CHAT
			_bAutoChat = false;
#endif
		}
		else if (IsUserEvent(event, UserEvent::LLMGenerationComplete))
		{
			auto pLLMInstance = Global::GetLLMInstance();
			if (pLLMInstance and _pVariableList)
				_pVariableList->SetVariables(pLLMInstance->GetStateVariables());
			NextQueuedCommand();
		}
		else if (IsUserEvent(event, UserEvent::ScreenActivated))
		{
			_pTextBox->SetFocus(true);
		}

		return Screen::OnEvent(event);
	}

	void ChatScreen::OnAfterLayout()
	{
		if (_pBehindChat)
		{
			_pBehindChat->SetHeight(GetHeight());
			_pBehindChat->CenterHorizontally();
		}
	}

}