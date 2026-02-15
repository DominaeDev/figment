#pragma once

#ifndef APPLICATION_STATE_H__
#define APPLICATION_STATE_H__

#include <memory>
#include "gui/GUITypes.h"

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Cursor;
enum SDL_SystemCursor : int;
struct TTF_TextEngine;

namespace fig::gui
{
	class Window;
}

namespace fig::fs
{
	class UserManager;
}

namespace fig::llm
{
	class LLMBackend;
	class LLMInstance;
}

class ApplicationState
{
public:
	struct State
	{
		std::shared_ptr<fig::gui::Window> pMainWindow;
		std::shared_ptr<fig::llm::LLMBackend> pLLMEngine;
		std::shared_ptr<fig::llm::LLMInstance> pLLMInstance;
		std::shared_ptr<fig::fs::UserManager> pUserManager;
	};

	static State* CreateState();
	static void ReleaseState();

	static fig::gui::Window& GetMainWindow();
	static fig::llm::LLMBackend& GetLLMEngine();
	static fig::fs::UserManager& GetUserManager();

	[[nodiscard]] static std::shared_ptr<fig::llm::LLMInstance> GetLLMInstance();
	static void SetLLMInstance(std::shared_ptr<fig::llm::LLMInstance> pLLMInstance);

	static void SetCursor(SDL_SystemCursor cursor);

private:
	static State* __appState;
	static SDL_Cursor* _pIBeamCursor;
};

typedef ApplicationState::State AppState;

#endif