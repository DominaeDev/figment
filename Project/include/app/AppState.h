#pragma once

#ifndef APPLICATION_STATE_H__
#define APPLICATION_STATE_H__

#include <memory>
#include "gui/GUITypes.h"
#include "app/AppSettings.h"
#include "user/UserSettings.h"

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

namespace fig::text
{
	class MacroProvider;
}

namespace fig
{
	class Global
	{
	public:
		struct State
		{
		private:
			friend Global;
			std::shared_ptr<fig::gui::Window> pMainWindow;
			std::shared_ptr<fig::llm::LLMBackend> pLLMBackend;
			std::shared_ptr<fig::llm::LLMInstance> pLLMInstance;
			std::shared_ptr<fig::user::UserManager> pUserManager;
			std::unique_ptr<fig::AppSettings> pAppSettings;
			std::shared_ptr<fig::text::MacroProvider> pMacroProvider;
			void Init();
			void Release();
		};

		static State* CreateState();
		static void ReleaseState();

		static fig::gui::Window& GetMainWindow();
		static fig::llm::LLMBackend& GetLLMBackend();
		static fig::AppSettings& GetSettings();
		
		static fig::user::UserManager& GetUserManager();
		static UserSettings& GetUserSettings();
		static fig::io::UserContentManager& GetUserContent();
		static std::weak_ptr<fig::text::MacroProvider> GetMacroProvider();

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