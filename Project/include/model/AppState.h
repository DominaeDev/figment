#pragma once

#ifndef __Application_h_
#define __Application_h_

#include <memory>

class MainFrame;
struct SDL_Window;
struct SDL_Renderer;
struct SDL_Cursor;
enum SDL_SystemCursor : int;
struct TTF_TextEngine;

class LLMEngine;
class LLMInstance;

class Application
{
public:
	struct State
	{
		SDL_Window* pWindow = nullptr;
		SDL_Renderer* pRenderer = nullptr;
		MainFrame* pTopFrame = nullptr;
		TTF_TextEngine* pTextEngine = nullptr;
		unsigned __int64 last_step {};

		std::shared_ptr<LLMEngine> pLLMEngine;
		std::shared_ptr<LLMInstance> pLLMInstance;
	};

	static State* CreateState();
	static void ReleaseState();

	static SDL_Window* GetWindow();
	static SDL_Renderer* GetRenderer();
	[[nodiscard]] static std::shared_ptr<LLMEngine> GetLLMEngine();
	[[nodiscard]] static std::shared_ptr<LLMInstance> GetLLMInstance();
	
	static void SetLLMInstance(std::shared_ptr<LLMInstance> pLLMInstance);

	static void SetCursor(SDL_SystemCursor cursor);

private:
	static State* __appState;

	static SDL_Cursor* _pIBeamCursor;

};

typedef Application::State AppState;

#endif