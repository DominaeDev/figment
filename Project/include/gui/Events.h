#ifndef EVENTS_H__
#define EVENTS_H__
#pragma once

#include "gui/GUITypes.h"

namespace fig::gui
{
	enum class UserEvent : uint32_t
	{
		UserSignedIn,
		UserSignedOut,
		MenuOpened,
		MenuClosed,
		Activated,
		Deactivated,

		LLMStatusUpdate,
		LLMModelLoading,
		LLMModelLoadingProgress,
		LLMModelLoaded,
		LLMModelLoadFailure,
		LLMModelUnloaded,
		LLMModelUnloadRequest,
		LLMChatInitializing,
		LLMChatInitialized,
		LLMChatInitializationFailure,
		LLMGenerationStarted,
		LLMGenerationComplete,
		LLMCompletedMessage,
		LLMRebuildingKVCache,

		StartChat,

		Count,
	};
	
	void RegisterUserEvents();
	extern uint32_t UserEventBase;

	inline constexpr uint32_t SDLUserEvent(UserEvent e) { return UserEventBase + static_cast<uint32_t>(e); }
	inline constexpr bool IsUserEvent(Event& event, UserEvent e) { return event.type == SDLUserEvent(e); }

	inline void PushEvent(UserEvent eventType, int32_t code = 0)
	{
		Event event {};
		event.type = SDLUserEvent(eventType);
		event.user.code = code;
		SDL_PushEvent(&event);
	}

	template <typename T>
	inline void PushEvent(UserEvent eventType, T* pData)
	{
		Event event {};
		event.type = SDLUserEvent(eventType);
		event.user.data1 = (void*)pData;
		SDL_PushEvent(&event);
	}

	template <typename T, typename U>
	inline void PushEvent(UserEvent eventType, T* pData1, U* pData2)
	{
		Event event {};
		event.type = SDLUserEvent(eventType);
		event.user.data1 = (void*)pData1;
		event.user.data2 = (void*)pData2;
		SDL_PushEvent(&event);
	}

	template <typename T>
	inline void PushEvent(UserEvent eventType, int32_t code, T* pData)
	{
		Event event {};
		event.type = SDLUserEvent(eventType);
		event.user.code = code;
		event.user.data1 = (void*)pData;
		SDL_PushEvent(&event);
	}

	template <typename T, typename U>
	inline void PushEvent(UserEvent eventType, int32_t code, T* pData1, U* pData2)
	{
		Event event {};
		event.type = SDLUserEvent(eventType);
		event.user.code = code;
		event.user.data1 = (void*)pData1;
		event.user.data2 = (void*)pData2;
		SDL_PushEvent(&event);
	}

	enum class EventResult
	{
		Pass,		// Not handled
		Continue,	// Handled, continue propagation
		Handled,	// Handled, stop propagation
	};
}
#endif
