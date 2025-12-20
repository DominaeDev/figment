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

namespace fig::llm
{
	class LLMEngine;
	class LLMInstance;
}

class ApplicationState
{
public:
	struct State
	{
		std::shared_ptr<fig::gui::Window> pMainWindow;
		std::shared_ptr<fig::llm::LLMEngine> pLLMEngine;
		std::shared_ptr<fig::llm::LLMInstance> pLLMInstance;
	};

	static State* CreateState();
	static void ReleaseState();

	static fig::gui::Window& GetMainWindow();
	static fig::llm::LLMEngine& GetLLMEngine();

	[[nodiscard]] static std::shared_ptr<fig::llm::LLMInstance> GetLLMInstance();
	static void SetLLMInstance(std::shared_ptr<fig::llm::LLMInstance> pLLMInstance);

	static void SetCursor(SDL_SystemCursor cursor);

private:
	static State* __appState;
	static SDL_Cursor* _pIBeamCursor;
};

typedef ApplicationState::State AppState;

#endif