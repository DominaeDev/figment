#pragma once

#include "Frame.h"
#include "model/ChatCommands.h"
#include "model/ChatCommandExecutor.h"
#include "llm/LLMStatus.h"

#include <unordered_map>

using ParsedChatCommandQueue = std::queue<ParsedChatCommand>;

namespace fig::llm
{
	struct LLMStatus;
}

namespace fig::gui
{
	class Screen;

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

		template <typename T>
			requires std::derived_from<T, fig::gui::Screen>
		T* GetScreen()
		{
			auto it = _screensByType.find(type_id<T>);
			if (it != _screensByType.end())
				return static_cast<T*>(it->second);
			return nullptr;
		}

		template <typename T>
			requires std::derived_from<T, fig::gui::Screen>
		const T* GetScreen() const
		{
			auto it = _screensByType.find(type_id<T>);
			if (it != _screensByType.end())
				return static_cast<T*>(it->second);
			return nullptr;
		}

		void Close();

		template<typename T>
		void ChangeScreen()
		{
			ChangeScreen(GetScreen<T>());
		}

	protected:
		template<typename T>
		void RegisterScreen();

		template<typename T>
		void UnregisterScreen();

		void ChangeScreen(Screen* pScreen);

		virtual void OnUpdate(float fDeltaTime) override;
		virtual void OnRender(Renderer* pRenderer) override;
		
		bool OnEvent(SDL_Event& event) override;

	private:
		std::unordered_map<type_id_t, Screen*> _screensByType {};
		Screen* _pActiveScreen = nullptr;

		StatusBar* _pStatusBar = nullptr;
		Control* _pMainArea = nullptr;
	};
}