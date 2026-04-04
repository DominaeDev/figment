#pragma once

#include "Frame.h"
#include "model/ChatCommands.h"
#include "model/ChatCommandExecutor.h"
#include "llm/LLMStatus.h"
#include "gui/Screen.h"

#include <unordered_map>

using ParsedChatCommandQueue = std::queue<ParsedChatCommand>;

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

	class MainFrame : public Frame
	{
		friend bool ChatCommandExecutor::Execute(ParsedChatCommand command, ChatCommandExecutor::Context context);
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
			auto it = _screensByType.find(type_id<T>());
			if (it != _screensByType.end())
				return static_cast<T*>(it->second);
			return nullptr;
		}

		template <IsScreen T>
		const T* GetScreen() const
		{
			auto it = _screensByType.find(type_id<T>());
			if (it != _screensByType.end())
				return static_cast<T*>(it->second);
			return nullptr;
		}

		void Close();

		template<IsScreen T>
		T* ChangeScreen()
		{
			T* pScreen = GetScreen<T>();
			ChangeScreen(pScreen);
			return pScreen;
		}

		void ShowLoginScreen();
		void ShowSidePanel(bool bShow) noexcept;

		bool TrySignIn(const fig::user::UserProfile& profile, const fig::string& password) noexcept;
		bool SignOut() noexcept;



	protected:
		template<IsScreen T>
		void RegisterScreen();

		template<IsScreen T>
		void UnregisterScreen();

		void ChangeScreen(Screen* pScreen);
		bool AutoSignIn() noexcept;

		virtual void OnUpdate(float fElapsed) override;
		virtual void OnRender(Renderer* pRenderer) override;
		
		bool OnEvent(SDL_Event& event) override;
		void OnSignedIn(const fig::user::UserProfile& profile) noexcept;
		void OnSignedOut() noexcept;

	private:
		std::unordered_map<type_id_t, Screen*> _screensByType {};
		Screen* _pActiveScreen = nullptr;

		StatusBar* _pStatusBar = nullptr;
		Control* _pMainArea = nullptr;
		SidePanel* _pSidePanel;
	};
}