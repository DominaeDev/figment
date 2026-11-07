#include <pch.h>
#include "gui/MainFrame.h"
#include "gui/CharacterScreen.h"
#include "gui/StoryScreen.h"
#include "gui/WorldScreen.h"
#include "gui/ChatScreen.h"
#include "gui/DebugScreen.h"
#include "gui/ChatBackground.h"
#include "gui/SidePanel.h"
#include "gui/LoginScreen.h"
#include "gui/EditorScreen.h"
#include "gui/editor/CharacterEditor.h"
#include "gui/editor/StoryEditor.h"
#include "gui/editor/WorldEditor.h"
#include "gui/editor/UserSettingsEditor.h"
#include "app/AppState.h"
#include "user/UserManager.h"
#include "io/FileUtility.h"
#include "io/CardImporter.h"
#include "llm/LLMBackend.h"
#include "llm/LLMInstance.h"
#include "llm/LLMUtility.h"
#include "data/ModelSettings.h"
#include "data/ChatInstance.h"
#include "tts/TTSBackend.h"

#include "util/DebugUtils.h"
#include "gui/ChatListingScreen.h" //! @temp
#include "gui/InfoPanel.h" //! @temp

using namespace fig::io;
using namespace fig::data;
using namespace fig::chat;
using namespace fig::user;
using namespace fig::tts;

namespace fig::gui
{
	MainFrame* MainFrame::s_pInstance = nullptr;

	MainFrame::MainFrame(Window* pWindow) : Frame(pWindow)
	{
		s_pInstance = this;

		SetForegroundColor(Color::AppBackground);
		SetBackgroundColor(Color::AppBackground);

		_pMainArea = CreateControl<Area>();
		_pSidePanel = CreateControl<SidePanel>();

		// Status bar
		_pStatusBar = CreateControl<StatusBar>();

		auto topSizer = SetSizer<VerticalSizer>();
		auto mainSizer = topSizer->Add<HorizontalSizer>(-1, SizerFlag::Expand);

		mainSizer->Add(_pSidePanel, 0, SizerFlag::Expand);
		mainSizer->Add(_pMainArea, -1, SizerFlag::Fill);

		topSizer->Add(_pStatusBar, 0, SizerFlag::Expand);

		RegisterScreen<LoginScreen>(ScreenType::Login);
		RegisterScreen<CharacterScreen>(ScreenType::Home);
		RegisterScreen<StoryScreen>(ScreenType::Stories);
		RegisterScreen<WorldScreen>(ScreenType::Worlds);
		RegisterScreen<ChatScreen>(ScreenType::Chat);
		RegisterScreen<DebugScreen>(ScreenType::Debug);
		RegisterScreen<ChatListingScreen>(ScreenType::ChatListing);
		RegisterScreen<EditorScreen>(ScreenType::Editor);

		DebugUtility::Initialize();

		// Sign in
		auto& userMngr = Global::GetUserManager();
		if (not userMngr.LoadProfiles())
			userMngr.CreateDefaultProfile();

		// Try auto sign-in
		if (not AutoSignIn())
			NavigateTo(ScreenType::Login);
	}

	MainFrame::~MainFrame()
	{
		for (auto& [_, pScreen] : _screensByType)
		{
			auto pParent = pScreen->GetParent();
			if (pParent)
				pParent->RemoveChild(pScreen);
			delete pScreen;
		}
		_screensByType.clear();
	}

	void MainFrame::OnUpdate(float fElapsed)
	{
		if (auto pLLM = Global::GetLLMInstance())
		{
			if (auto pSession = pLLM->GetSession().lock())
				pSession->Update(fElapsed);
		}
	}

	void MainFrame::OnRender(fig::renderer_ptr pRenderer)
	{
		DrawBackground(pRenderer);
	}

	void MainFrame::SetStatusBar(fig::string_view message)
	{
		s_pInstance->_pStatusBar->SetMessage(toStr(message));
	}

	void MainFrame::SetStatusBar(const fig::llm::LLMStatus& status)
	{
		s_pInstance->_pStatusBar->SetModelInfo(status);
	}

	void MainFrame::Close()
	{
		SDL_Event quit_event;
		SDL_zero(quit_event);
		quit_event.type = SDL_EVENT_QUIT;
		SDL_PushEvent(&quit_event);
	}

	void MainFrame::ChangeScreen(ScreenType screen)
	{
		Screen* pScreen = nullptr;
		if (auto itFind = _screensByType.find(screen); itFind != _screensByType.end())
			pScreen = itFind->second;

		PushEvent(UserEvent::LeaveScreen, &_currentScreen);

		if (_pActiveScreen)
		{
			_pActiveScreen->SetVisible(false);
			_pActiveScreen->PushEvent(UserEvent::ScreenDeactivated);
			
			// Detach
			auto pParent = _pActiveScreen->GetParent();
			if (pParent)
				pParent->RemoveChild(pScreen);
			_pActiveScreen = nullptr;
		}

		_pActiveScreen = pScreen;
		if (not (bool)_pActiveScreen)
			return;

		_currentScreen = screen;
		_pMainArea->RemoveChildren();
		_pMainArea->AddChild(_pActiveScreen);
		
		auto sizer = _pMainArea->SetSizer<VerticalSizer>();
		sizer->Add(_pActiveScreen, -1, SizerFlag::Fill);

		_pActiveScreen->SetVisible(true);
		_pActiveScreen->PushEvent(UserEvent::ScreenActivated);
		PushEvent(UserEvent::ChangedScreen, &_currentScreen);

		InvalidateLayout();

		// Reset cursor
		ResetCursor();
	}

	template<IsScreen T>
	void MainFrame::RegisterScreen(ScreenType screen)
	{
		if (_screensByType.contains(screen))
			UnregisterScreen(screen);

		auto pScreen = new T(this);	// Must pass this to receive renderer
		RemoveChild(pScreen);
		_screensByType[screen] = pScreen;
		pScreen->SetSize(GetSize());
	}

	void MainFrame::UnregisterScreen(ScreenType screen)
	{
		if (auto itFind = _screensByType.find(screen); itFind != _screensByType.end())
		{
			auto pScreen = itFind->second;
			_pMainArea->RemoveChild(pScreen);
			RemoveChild(pScreen);
			_screensByType.erase(screen);
			delete pScreen;
		}
	}

	void MainFrame::ShowSidePanel(bool bShow) noexcept
	{
		if (_pSidePanel->GetVisible() != bShow)
		{
			_pSidePanel->SetVisible(bShow);
			_pSidePanel->EnableLayout(bShow);
			_pSidePanel->Cull(!bShow);
			InvalidateLayout();
		}
	}

	bool MainFrame::TrySignIn(const fig::user::UserProfile& profile, fig::string_view password) noexcept
	{
		auto& userMngr = Global::GetUserManager();
		auto startTime = std::chrono::steady_clock::now();

		if (userMngr.SignIn(profile.id, fig::string { password }))
		{
			OnSignedIn(profile);

			auto endTime = std::chrono::steady_clock::now();
			// MainFrame::SetStatusBar(std::format("Duration: {}ms", toD(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count())));
			return true;
		}

		return false;
	}

	bool MainFrame::SignOut() noexcept
	{
		auto& userMngr = Global::GetUserManager();
		if (userMngr.SignOut())
		{
			AppColors::SetTheme(ColorTheme::SystemDefault, false);
			OnSignedOut();
			return true;
		}
		return false;
	}

	void MainFrame::OnSignedIn(const fig::user::UserProfile& profile) noexcept
	{
		Global::GetSettings().SetUUID(AppSetting::LastUser, profile.id);
		Global::GetSettings().SetBool(AppSetting::SignedIn, true);

		NavigateHome();
		PushEvent(UserEvent::UserSignedIn, &profile);

		auto theme = Global::GetUserSettings().GetColorTheme();
		AppColors::SetTheme(theme, false);
		PushEvent(UserEvent::ColorThemeChanged);
	}

	void MainFrame::OnSignedOut() noexcept
	{
		PushEvent(UserEvent::UserSignedOut);
		NavigateHome();

		Global::GetSettings().SetBool(AppSetting::SignedIn, false);
	}

	bool MainFrame::AutoSignIn() noexcept
	{
		if (not Global::GetSettings().GetBool(AppSetting::SignedIn))
			return false;

		auto& profiles = Global::GetUserManager().GetProfiles();
		if (profiles.empty())
			return false;

		if (auto lastProfile = Global::GetUserManager().GetProfile(Global::GetSettings().GetUUID(AppSetting::LastUser)))
		{
			if ((*lastProfile).state == UserProfile::State::Open)
				return TrySignIn(*lastProfile, "");
			return false;
		}

		return TrySignIn(profiles.front(), "");
	}

	void MainFrame::InitializeModel()
	{
		auto& backend = Global::GetLLMBackend();

		if (!backend.IsInitialized() && !backend.IsInitializing())
		{
			SetStatusBar(fig::strings::Status::LoadingModel);

			// Read model settings
			fig::data::ModelSettings modelSettings {};
			if (auto try_model_settings = Global::GetUserContent().GetActiveModelSettings())
				modelSettings = try_model_settings.value();
			else
			{
				LogLn("No model settings.");
				PushEvent(UserEvent::LLMModelLoadFailure);
				return;
			}

			backend.Initialize(modelSettings,
				[this](int percent) {
					PushEvent(UserEvent::LLMModelLoadingProgress, percent);
				},
				[this, &backend](bool bSuccess) {
					if (bSuccess)
					{
						auto pInstance = backend.CreateInstance();
						Global::SetLLMInstance(pInstance);
					}
				});
		}
	}

	void MainFrame::UnloadModel()
	{
		auto& backend = Global::GetLLMBackend();
		if (backend.IsInitialized())
		{
			backend.Shutdown();
			SetStatusBar(fig::strings::Status::ModelUnloaded);
		}
	}

	EventResult MainFrame::OnEvent(fig::event& event)
	{
		if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP)
		{
			SDL_KeyboardEvent& keyEvent = event.key;
			KeyboardMods mods { event };

			if (keyEvent.down and not keyEvent.repeat)
			{
				if constexpr (Debugging)
				{
					if (keyEvent.key == SDLK_F12 and mods.Control)
					{
						Close();
						return EventResult::Handled;
					}
					else if (keyEvent.key == SDLK_4 and mods.Alt)
					{
						NavigateTo(ScreenType::Debug);
						return EventResult::Handled;
					}
					else if (keyEvent.key == SDLK_F12 and mods.Control)
					{
						Close();
						return EventResult::Handled;
					}
					else if (keyEvent.key == SDLK_1 and mods.Alt)
					{
						NavigateHome();
						return EventResult::Handled;
					}
					else if (keyEvent.key == SDLK_2 and mods.Alt)
					{
						if constexpr (Debugging) //! @temp
						{
							NavigateTo(ScreenType::Chat);
							if (auto pChatScreen = GetScreen<ChatScreen>())
							{
								auto characterId = fig::uuid { Constants::Debug::TestCharacterId };
								StartChat(characterId);
							}
							return EventResult::Handled;
						}
					}
					else if (keyEvent.key == SDLK_3 and mods.Alt)
					{
						return EventResult::Handled;
					}
					else if (keyEvent.key == SDLK_F2 and mods.None)
					{
						InitializeModel();
						return EventResult::Handled;
					}
					else if (keyEvent.key == SDLK_F3 and mods.None)
					{
						UnloadModel();
						return EventResult::Handled;
					}
					else if (keyEvent.key == SDLK_F4 and mods.None)
					{
						Global::GetTTSBackend().Initialize();
						auto _discarded = Global::GetTTSBackend().Speak(fig::uuid { Constants::Debug::TestCharacterId }, ".");
						return EventResult::Handled;
					}
					else if (keyEvent.key == SDLK_F4 and mods.Control)
					{
						Global::GetTTSBackend().Shutdown();
						return EventResult::Handled;
					}
					else if (keyEvent.key == SDLK_F1 and mods.None)
					{
						if (Global::IsSignedIn())
						{
							int32_t theme = static_cast<int32_t>(Global::GetUserSettings().GetColorTheme());
							if (++theme == static_cast<int32_t>(ColorTheme::Count))
								theme = static_cast<int32_t>(ColorTheme::LightDefault);
							ChangeColorTheme(static_cast<ColorTheme>(theme), true);
							return EventResult::Handled;
						}
					}
					else if (keyEvent.key == SDLK_F1 and mods.Control)
					{
						// Reload colors
						auto currentTheme = AppColors::GetTheme();
						AppColors::Init();
						AppColors::SetTheme(currentTheme, false);
						PushEvent(UserEvent::ColorThemeChanged);
						return EventResult::Handled;
					}
				}

			}
		}

		if (IsUserEvent(event, UserEvent::StartChat))
		{
			auto& characterId = GetUserData<fig::uuid>(event);
			StartChat(characterId);
			return EventResult::Handled;
		}
		else if (IsUserEvent(event, UserEvent::Scrolling))
		{
			PopAllOverlays();
			return EventResult::Continue;
		}
		else if (IsUserEvent(event, UserEvent::NavigateToHome))
		{
			NavigateHome();
			return EventResult::Handled;
		}
		else if (IsUserEvent(event, UserEvent::NavigateBack))
		{
			NavigateBack();
			return EventResult::Handled;
		}
		else if (IsUserEvent(event, UserEvent::NavigateToStories))
		{
			NavigateTo(ScreenType::Stories);
			return EventResult::Handled;
		}
		else if (IsUserEvent(event, UserEvent::NavigateToWorlds))
		{
			NavigateTo(ScreenType::Worlds);
			return EventResult::Handled;
		}
		else if (IsUserEvent(event, UserEvent::NavigateToChatList))
		{
			NavigateTo(ScreenType::ChatListing);
			if (HasUserData1(event))
			{
				auto& characterId = GetUserData1<fig::uuid>(event);
				bool bShowHidden = (bool)event.user.code;
				GetScreen<ChatListingScreen>()->ShowChatsWith(characterId, bShowHidden);
			}
			else
			{
				bool bShowHidden = (bool)event.user.code;
				GetScreen<ChatListingScreen>()->ShowAllChats(bShowHidden);
			}
			return EventResult::Handled;
		}
		else if (IsUserEvent(event, UserEvent::ChangedScreen))
		{
			auto& screen = GetUserData1<ScreenType>(event);
			switch (screen)
			{
			case ScreenType::Login:
				ShowSidePanel(false);
				break;
			default:
				ShowSidePanel(true);
				break;
			}
			PopAllOverlays();
			return EventResult::Continue;
		}
		else if (IsUserEvent(event, UserEvent::SidePanelResized))
		{
			PopAllOverlays();
			InvalidateLayout();
			return EventResult::Continue;
		}
		else if (IsUserEvent(event, UserEvent::LLMModelUnloadRequest))
		{
			UnloadModel();
			return EventResult::Continue;
		}
		else if (IsUserEvent(event, UserEvent::TTSServerShutdown))
		{
			Global::GetTTSBackend().Shutdown();
			return EventResult::Continue;
		}
		else if (IsUserEvent(event, UserEvent::CreateCharacter))
		{
			NavigateTo(ScreenType::Editor, NavigationState::Context::Character, {}, true);
			return EventResult::Handled;
		}
		else if (IsUserEvent(event, UserEvent::ImportCharacter))
		{
			if (auto fn = GetEventText(event))
			{
				auto paths = decode_csv(*fn)
					| std::views::transform([](auto&& s) { return std::filesystem::u8path(s); })
					| std::ranges::to<std::vector>();
				
				if (paths.empty())
					return EventResult::Handled;

				size_t num_imported = 0uz;
				for (auto& path : paths)
				{
					if (Global::GetUserContent().ImportCharacter(path))
						num_imported++;
				}
				size_t num_failed = paths.size() - num_imported;
				if (num_imported)
				{
					if (num_imported < paths.size())
					{
						fig::string message = std::format("Successfully imported {} character{}.", num_imported, num_imported > 1 ? "s" : "");
						message += std::format("\n{} files{} could not be imported.", num_failed, num_failed > 1 ? "s" : "");
						ShowDialogBox(message, DialogBoxType::Ok);
					}

					PushEvent(UserEvent::AddedCharacter);
				}
				else
					ShowDialogBox("Failed to import character.", DialogBoxType::Ok);
			}
			else
				CardImporter::ShowImportDialog(GetSDLWindow());

			return EventResult::Handled;
		}
		else if (IsUserEvent(event, UserEvent::EditCharacter))
		{
			const fig::uuid& characterId = GetUserData<fig::uuid>(event);
			NavigateTo(ScreenType::Editor, NavigationState::Context::Character, characterId, true);
			return EventResult::Handled;
		}
		else if (IsUserEvent(event, UserEvent::CreateStory))
		{
			NavigateTo(ScreenType::Editor, NavigationState::Context::Scenario, {}, true);
			return EventResult::Handled;
		}
		else if (IsUserEvent(event, UserEvent::EditStory))
		{
			const fig::uuid& scenarioId = GetUserData<fig::uuid>(event);
			NavigateTo(ScreenType::Editor, NavigationState::Context::Scenario, scenarioId, true);
			return EventResult::Handled;
		}
		else if (IsUserEvent(event, UserEvent::CreateWorld))
		{
			NavigateTo(ScreenType::Editor, NavigationState::Context::World, {}, true);
			return EventResult::Handled;
		}
		else if (IsUserEvent(event, UserEvent::EditWorld))
		{
			const fig::uuid& worldId = GetUserData<fig::uuid>(event);
			NavigateTo(ScreenType::Editor, NavigationState::Context::World, worldId, true);
			return EventResult::Handled;
		}
		else if (IsUserEvent(event, UserEvent::EditCharacterStory))
		{
			const fig::uuid& characterId = GetUserData<fig::uuid>(event);
			NavigateTo(ScreenType::Editor, NavigationState::Context::CharacterScenario, characterId, true);
			return EventResult::Handled;
		}
		else if (IsUserEvent(event, UserEvent::EditUserSettings))
		{
			NavigateTo(ScreenType::Editor, NavigationState::Context::UserSettings, {}, true);
			return EventResult::Handled;
		}
		else if (IsUserEvent(event, UserEvent::StartTextInput)
			and _pFocusedControl != event.user.data1)
		{
			if (!_pFocusedControl)
				SDL_StartTextInput(GetSDLWindow());
			_pFocusedControl = event.user.data1;
		}
		else if (IsUserEvent(event, UserEvent::StopTextInput)
			and _pFocusedControl == event.user.data1)
		{
			if (_pFocusedControl)
				SDL_StopTextInput(GetSDLWindow());
			_pFocusedControl = {};
		}
		else if (IsUserEvent(event, UserEvent::ProfileDeleted))
		{
			OnSignedOut();
			return EventResult::Handled;
		}

		HandleStatusBarEvents(event);

		if constexpr (Debugging)
		{
			if (IsUserEvent(event, UserEvent::DebugCharacter))
			{
				const fig::uuid& characterId = GetUserData<fig::uuid>(event);
				DebugUtility::DebugCharacter(characterId);
				return EventResult::Handled;
			}
		}

		if (_pSidePanel and _pSidePanel->GetVisible())
		{
			if (auto result = _pSidePanel->ProcessEvent(event); result == EventResult::Handled)
				return EventResult::Handled;
		}

		if (_pStatusBar)
		{
			if (auto result = _pStatusBar->ProcessEvent(event); result == EventResult::Handled)
				return EventResult::Handled;
		}

		if (IsBroadcastEvent(event))
		{
			// Pass to all screens
			BroadcastEvent(event);
			return EventResult::Continue;
		}
		else
		{
			// Pass to active screen only
			if (_pActiveScreen)
				return _pActiveScreen->ProcessEvent(event);
		}
		
		return EventResult::Pass;
	}

	bool MainFrame::HandleStatusBarEvents(fig::event& event)
	{
		if (IsUserEventWithData(event, UserEvent::LLMStatusUpdate))
		{
			SetStatusBar(GetUserData<fig::llm::LLMStatus>(event));
			return true;
		}
		else if (IsUserEvent(event, UserEvent::LLMChatInitializing))
		{
			SetStatusBar(fig::strings::Status::InitializingChat);
			return true;
		}
		else if (IsUserEvent(event, UserEvent::LLMChatInitialized))
		{
			SetStatusBar(fig::strings::Status::ChatInitialized);
			return true;
		}
		else if (IsUserEvent(event, UserEvent::LLMChatInitializationFailure))
		{
			SetStatusBar(fig::strings::Status::FailedToInitializeChat);
			return true;
		}
		else if (IsUserEvent(event, UserEvent::LLMModelLoading))
		{
			SetStatusBar(fig::strings::Status::LoadingModel);
			return true;
		}
		else if (IsUserEvent(event, UserEvent::LLMModelLoaded))
		{
			SetStatusBar(fig::strings::Status::ModelLoaded);
			return true;
		}
		else if (IsUserEvent(event, UserEvent::LLMModelUnloaded))
		{
			SetStatusBar(fig::strings::Status::ModelUnloaded);
			return true;
		}
		else if (IsUserEvent(event, UserEvent::LLMModelLoadFailure))
		{
			SetStatusBar(fig::strings::Status::FailedToLoadModel);
			return true;
		}
		else if (IsUserEvent(event, UserEvent::LLMGenerationStarted))
		{
			SetStatusBar(fig::strings::Status::GeneratingResponse);
			return true;
		}
		else if (IsUserEvent(event, UserEvent::LLMRebuildingKVCache))
		{
			SetStatusBar(fig::strings::Status::RebuildingContext);
			return true;
		}
		else if (IsUserEvent(event, UserEvent::LLMGenerationComplete))
		{
			SetStatusBar(fig::strings::Status::Ready);
			return true;
		}
		return false;
	}

	EventResult MainFrame::BroadcastEvent(fig::event& event)
	{
		EventResult result { EventResult::Pass };
		
		// Pass to all screens
		for (auto& kvp : _screensByType)
		{
			Screen* pScreen = kvp.second;
			result = std::max(result, pScreen->ProcessEvent(event));
			if (result == EventResult::Handled)
				return EventResult::Handled;
		}

		return result;
	}

	bool MainFrame::StartChat(const fig::uuid& characterId)
	{
		if (auto character = Global::GetUserContent().Get<Character>(characterId))
		{
			// Preload speech model
			Global::GetTTSBackend().UnloadDesignModels(); // jic
			if (Global::GetUserSettings().GetBool(UserSetting::TTS::Warmup))
			{
				auto _discarded = Global::GetTTSBackend().Speak(characterId, ".");
			}

			PromptScaffold scaffold;
			if (!Success(scaffold.LoadFromXml(fig::path(Constants::Paths::PromptScaffold))))
				return false;

			Scenario scenario;
			fig::uuid scenarioId;
			if (auto try_scenario = Global::GetUserContent().GetCharacterScenario(characterId))
			{
				scenario = (*try_scenario).instance;
				scenarioId = (*try_scenario).assetId;
			}
			else
			{
				if (!Success(scenario.LoadFromXml(fig::path(Constants::Paths::DefaultScenario))))
					return false;
			}

			ChatStaging staging(scenario, scenarioId, scaffold, Constants::LLM::DefaultChatOptions);

			if (!staging.AddCharacter(Role::Bot1, characterId, character.value()))
				return false;

			Character user;
			if (not (Success(user.LoadFromXml(fig::path { "./characters/user.xml" })) and staging.AddCharacter(Role::User, _CreateUUID(), user))) //! @temp
				return false;

			// Create instance
			auto [chatInstanceId, chatLogId] = Global::GetUserContent().CreateChat(staging);  

			NavigateTo(ScreenType::Chat);
			auto pChatScreen = GetScreen<ChatScreen>();
			pChatScreen->StartChat(staging, chatInstanceId, chatLogId);
			return true;
		}
		return false;
	}

	void MainFrame::ChangeColorTheme(ColorTheme theme, bool bTransition)
	{
		if (Global::IsSignedIn())
			Global::GetUserSettings().SetColorTheme(theme);
		AppColors::SetTheme(theme, bTransition);
		PushEvent(UserEvent::ColorThemeChanged);
	}
	
	void MainFrame::NavigateTo(ScreenType screen, NavigationState::Context context, const fig::uuid& assetId, bool bPush)
	{
		if (not bPush)
			stack_clear(_navigation);

		_navigation.push(NavigationState {
			.screen = screen,
			.context = context,
			.assetId = assetId,
		});

		switch (screen)
		{
		case ScreenType::Editor:
			if (Global::GetUserManager().IsSignedIn())
			{
				if (context == NavigationState::Context::Character)
				{
					auto pEditor = ChangeScreen<EditorScreen>()->SetEditor<CharacterEditor>();
					if (assetId.empty() and pEditor->NewCharacter())
						return;
					if (not assetId.empty() and pEditor->EditCharacter(assetId))
						return;
				}
				else if (context == NavigationState::Context::CharacterScenario)
				{
					auto pEditor = ChangeScreen<EditorScreen>()->SetEditor<StoryEditor>();
					if (not assetId.empty() and pEditor->EditCharacterStory(assetId))
						return;
				}
				else if (context == NavigationState::Context::Scenario)
				{
					auto pEditor = ChangeScreen<EditorScreen>()->SetEditor<StoryEditor>();
					if (assetId.empty() and pEditor->NewStory())
						return;
					if (not assetId.empty() and pEditor->EditStory(assetId))
						return;
				}
				else if (context == NavigationState::Context::World)
				{
					auto pEditor = ChangeScreen<EditorScreen>()->SetEditor<WorldEditor>();
					if (assetId.empty() and pEditor->NewWorld())
						return;
					if (not assetId.empty() and pEditor->EditWorld(assetId))
						return;
				}
				else if (context == NavigationState::Context::UserSettings)
				{
					auto pEditor = ChangeScreen<EditorScreen>()->SetEditor<UserSettingsEditor>();
					if (pEditor->Initialize({}))
						return;
				}
			}
			break;
		case ScreenType::Login:
			ChangeScreen(ScreenType::Login);
			ShowSidePanel(false);
			return;
		default:
			if (Global::GetUserManager().IsSignedIn())
			{
				ChangeScreen(screen);
				return;
			}
			break;
		}

		// Error
		if (bPush)
			NavigateBack();
	}

	void MainFrame::NavigateBack()
	{
		_navigation.pop();

		if (_navigation.empty())
		{
			NavigateHome();
			return;
		}
		else
		{
			NavigationState state = _navigation.top();
			NavigateTo(state.screen, state.context, state.assetId, false);
		}
	}

	void MainFrame::NavigateHome()
	{
		for (; not _navigation.empty();)
			_navigation.pop();

		if (Global::IsSignedIn())
			NavigateTo(ScreenType::Home);
		else
			NavigateTo(ScreenType::Login);
	}
}