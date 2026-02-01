#pragma once

#include "Frame.h"
#include "model/ChatCommands.h"
#include "model/ChatCommandExecutor.h"
#include "llm/LLMStatus.h"

using ParsedChatCommandQueue = std::queue<ParsedChatCommand>;

namespace fig::llm
{
	struct LLMStatus;
}

namespace fig::gui
{
	class Screen;
	class HomeFrame;
	class ChatFrame;

	enum class ScreenType
	{
		Home,
		Chat,
	};

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

		HomeFrame& GetHomeFrame() { return *_pHomeFrame; }
		const HomeFrame& GetHomeFrame() const { return *_pHomeFrame; }
		ChatFrame& GetChatFrame() { return *_pChatFrame; }
		const ChatFrame& GetChatFrame() const { return *_pChatFrame; }

		void Close();

		void ChangeScreen(ScreenType screen);

	protected:
		virtual void OnUpdate(float fDeltaTime) override;
		virtual void OnRender(Renderer* pRenderer) override;
		
		bool OnEvent(Event& event) override;

	private:
		HomeFrame* _pHomeFrame = nullptr;
		ChatFrame* _pChatFrame = nullptr;
		Screen* _pActiveScreen = nullptr;

		StatusBar* _pStatusBar = nullptr;
		Control* _pMainArea = nullptr;
	};
}