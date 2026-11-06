#pragma once

#include "Frame.h"
#include "chat/ChatCommands.h"
#include "chat/ChatCommandExecutor.h"
#include "llm/LLMStatus.h"
#include "gui/Screen.h"

#include <unordered_map>
#include <stack>

namespace fig::llm
{
	struct LLMStatus;
}

namespace fig::user
{
	struct UserProfile;
}

namespace fig::gui
{
	class SidePanel;
	enum class ColorTheme;

	struct NavigationState
	{
		ScreenType screen { ScreenType::Undefined };
		enum class Context { None, Character, CharacterScenario, Scenario, World, UserSettings, } context {};
		fig::uuid assetId {};
	};

	class MainFrame : public Frame
	{
		friend bool fig::chat::ChatCommandExecutor::Execute(fig::chat::ParsedChatCommand command, fig::chat::ChatCommandExecutor::Context context);
		static MainFrame* s_pInstance;
	public:
		MainFrame(Window* pWindow);
		~MainFrame();

		static void SetStatusBar(fig::string_view message);
		static void SetStatusBar(const fig::llm::LLMStatus& status);
		static MainFrame& GetInstance() { return *s_pInstance; }

		template <IsScreen T>
		T* GetScreen()
		{
			auto it = _screensByType.find(ScreenTypeOf<T>);
			if (it != _screensByType.end())
				return dynamic_cast<T*>(it->second);
			return nullptr;
		}

		template <IsScreen T>
		const T* GetScreen() const
		{
			auto it = _screensByType.find(ScreenTypeOf<T>);
			if (it != _screensByType.end())
				return dynamic_cast<T*>(it->second);
			return nullptr;
		}

		void ShowSidePanel(bool bShow) noexcept;
		fig::observer_ptr<SidePanel> GetSidePanel() noexcept { return _pSidePanel; }

		bool TrySignIn(const fig::user::UserProfile& profile, fig::string_view password) noexcept;
		bool SignOut() noexcept;
		void Close();

		void InitializeModel();
		void UnloadModel();

		void ChangeColorTheme(ColorTheme theme, bool bTransition = false);

		void NavigateTo(ScreenType screen, NavigationState::Context context = {}, const fig::uuid& assetId = {}, bool bPush = false);
		void NavigateBack();
		void NavigateHome();

	protected:
		template<IsScreen T>
		void RegisterScreen(ScreenType screen);
		void UnregisterScreen(ScreenType screen);
		void ChangeScreen(ScreenType screen);

		template <IsScreen T>
		const T* ChangeScreen() const
		{
			ChangeScreen(ScreenTypeOf<T>);
			return GetScreen<T>();
		}

		template <IsScreen T>
		T* ChangeScreen()
		{
			ChangeScreen(ScreenTypeOf<T>);
			return GetScreen<T>();
		}

		EventResult BroadcastEvent(fig::event& event);
		bool HandleStatusBarEvents(fig::event& event);

		bool AutoSignIn() noexcept;
		bool StartChat(const fig::uuid& characterId);

		virtual void OnUpdate(float fElapsed) override;
		virtual void OnRender(fig::renderer_ptr pRenderer) override;
		
		EventResult OnEvent(fig::event& event) override;
		void OnSignedIn(const fig::user::UserProfile& profile) noexcept;
		void OnSignedOut() noexcept;

	private:
		std::unordered_map<ScreenType, Screen*> _screensByType {};
		fig::observer_ptr<Screen> _pActiveScreen;
		fig::observer_ptr<StatusBar> _pStatusBar;
		fig::observer_ptr<Control> _pMainArea;
		fig::observer_ptr<SidePanel> _pSidePanel;
		fig::opaque_ptr _pFocusedControl {};
		ScreenType _currentScreen {};
		
		std::stack<NavigationState> _navigation {};
	};
}