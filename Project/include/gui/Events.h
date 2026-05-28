#ifndef EVENTS_H__
#define EVENTS_H__
#pragma once

#include "gui/GUITypes.h"

namespace fig::gui
{
	enum UserEvent : uint32_t
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

	inline uint32_t SDLUserEvent(UserEvent e) { return UserEventBase + static_cast<uint32_t>(e); }
	inline bool IsUserEvent(Event& event, UserEvent e) { return event.type == SDLUserEvent(e); }

	inline void PushEvent(UserEvent eventType, int32_t code = 0, void* pData1 = nullptr, void* pData2 = nullptr)
	{
		Event event {};
		event.type = SDLUserEvent(eventType);
		event.user.code = code;
		event.user.data1 = pData1;
		event.user.data2 = pData2;
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

	enum class EventResult
	{
		Pass,		// Not handled
		Continue,	// Handled, continue propagation
		Handled,	// Handled, stop propagation
	};
}
#endif
