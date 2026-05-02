#pragma once

#ifndef APPLICATION_STATE_H__
#define APPLICATION_STATE_H__

#include <memory>
#include "gui/GUITypes.h"
#include "model/AppSettings.h"
#include "model/UserSettings.h"

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Cursor;
enum SDL_SystemCursor : int;
struct TTF_TextEngine;

namespace fig::gui
{
	class Window;
}

namespace fig::user
{
	class UserManager;
}

namespace fig::io
{
	class UserContentManager;
}

namespace fig::llm
{
	class LLMBackend;
	class LLMInstance;
}

namespace fig
{
	class Global
	{
	public:
		struct State
		{
			std::shared_ptr<fig::gui::Window> pMainWindow;
			std::shared_ptr<fig::llm::LLMBackend> pLLMBackend;
			std::shared_ptr<fig::llm::LLMInstance> pLLMInstance;
			std::shared_ptr<fig::user::UserManager> pUserManager;
			std::unique_ptr<fig::AppSettings> pAppSettings;
		};

		static State* CreateState();
		static void ReleaseState();

		static fig::gui::Window& GetMainWindow();
		static fig::llm::LLMBackend& GetLLMEngine();
		static fig::AppSettings& GetSettings();
		
		static fig::user::UserManager& GetUserManager();
		static fig::io::UserContentManager& GetUserContent();
		static fig::UserSettings& GetUserSettings();

		[[nodiscard]] static std::shared_ptr<fig::llm::LLMInstance> GetLLMInstance();
		static void SetLLMInstance(std::shared_ptr<fig::llm::LLMInstance> pLLMInstance);
		static bool IsLLMInitialized();

		static void SetCursor(SDL_SystemCursor cursor);

	private:
		static State* __appState;
		static SDL_Cursor* _pIBeamCursor;
	};

	using AppState = Global::State;
}


#endif