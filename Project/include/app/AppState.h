#pragma once

#include <memory>
#include "gui/GUITypes.h"
#include "app/AppSettings.h"
#include "user/UserSettings.h"
#include "tts/ITTSBackend.h"

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
			std::unique_ptr<fig::io::AppSettings> pAppSettings;
			std::shared_ptr<fig::text::MacroProvider> pMacroProvider;
			std::unique_ptr<std::map<fig::cursor, fig::sdl::Cursor>> pSystemCursors;
			std::unique_ptr<fig::tts::ITTSBackend> pTTSBackend;

			void Init();
			void Release();
			void CreateCursor(SDL_SystemCursor sdl_cursor);
		};

		static State* CreateState();
		static void ReleaseState();

		static fig::gui::Window& GetMainWindow();
		static fig::llm::LLMBackend& GetLLMBackend();
		static fig::tts::ITTSBackend& GetTTSBackend();
		static fig::io::AppSettings& GetSettings();
		
		static fig::user::UserManager& GetUserManager();
		static fig::io::UserSettings& GetUserSettings();
		static fig::io::UserContentManager& GetUserContent();
		static std::weak_ptr<fig::text::MacroProvider> GetMacroProvider();

		static std::shared_ptr<fig::llm::LLMInstance> GetLLMInstance();
		static void SetLLMInstance(std::shared_ptr<fig::llm::LLMInstance> pLLMInstance);
		static bool IsLLMInitialized();
		static bool IsSignedIn();

		static void SetCursor(fig::cursor cursor);

	private:
		static State* __appState;
	};

	using AppState = Global::State;
}