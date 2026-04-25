#ifndef EVENTS_H__
#define EVENTS_H__
#pragma once

#include "gui/GUITypes.h"

namespace fig::gui
{
	enum EventType : uint32_t
	{
		UserSignedIn,
		UserSignedOut,
		MenuOpened,
		MenuClosed,

		LLMStatusUpdate,
		LLMModelLoading,
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

		Count,
	};
	
	extern void RegisterUserEvents();
	extern uint32_t USER_EVENT_BASE;

#define USER_EVENT(X) (USER_EVENT_BASE + X)

	inline void PushEvent(EventType eventType, int32_t code = 0, void* pData1 = nullptr, void* pData2 = nullptr)
	{
		Event event {};
		event.type = USER_EVENT(eventType);
		event.user.code = code;
		event.user.data1 = pData1;
		event.user.data2 = pData2;
		SDL_PushEvent(&event);
	}

	template <typename T>
	inline void PushEvent(EventType eventType, T* pData)
	{
		Event event {};
		event.type = USER_EVENT(eventType);
		event.user.data1 = (void*)pData;
		SDL_PushEvent(&event);
	}
}
#endif
