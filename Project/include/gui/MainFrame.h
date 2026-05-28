#pragma once

#include "Frame.h"
#include "chat/ChatCommands.h"
#include "chat/ChatCommandExecutor.h"
#include "llm/LLMStatus.h"
#include "gui/Screen.h"

#include <unordered_map>

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

	enum class ScreenType : size_t
	{
		Debug,
		Login,
		Home,
		Chat,
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
		T* GetScreen(ScreenType screen)
		{
			auto it = _screensByType.find(screen);
			if (it != _screensByType.end())
				return dynamic_cast<T*>(it->second);
			return nullptr;
		}

		template <IsScreen T>
		const T* GetScreen(ScreenType screen) const
		{
			auto it = _screensByType.find(screen);
			if (it != _screensByType.end())
				return dynamic_cast<T*>(it->second);
			return nullptr;
		}
		void ChangeScreen(ScreenType screen);

		void ShowLoginScreen();
		void ShowSidePanel(bool bShow) noexcept;

		bool TrySignIn(const fig::user::UserProfile& profile, const fig::string& password) noexcept;
		bool SignOut() noexcept;
		void Close();

		void InitializeModel();
		void UnloadModel();

	protected:
		template<IsScreen T>
		void RegisterScreen(ScreenType screen);
		void UnregisterScreen(ScreenType screen);

		bool AutoSignIn() noexcept;
		bool StartChat(const fig::uuid& characterId);

		virtual void OnUpdate(float fElapsed) override;
		virtual void OnRender(Renderer* pRenderer) override;
		
		EventResult OnEvent(Event& event) override;
		void OnSignedIn(const fig::user::UserProfile& profile) noexcept;
		void OnSignedOut() noexcept;

	private:
		std::unordered_map<ScreenType, Screen*> _screensByType {};
		Screen* _pActiveScreen = nullptr;

		StatusBar* _pStatusBar = nullptr;
		Control* _pMainArea = nullptr;
		SidePanel* _pSidePanel;
	};
}