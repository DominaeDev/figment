#pragma once

#include "gui/GUITypes.h"

namespace fig::gui
{
	enum class UserEvent : uint32_t
	{
		/* UI events*/
		PushCursor,
		PopCursor,
		MenuOpened,
		MenuClosed,
		Activated,
		Deactivated,
		Scrolling,
		SidePanelResized,
		NavigateToChatList,
		StartChat,

		EditCharacter,
		DebugCharacter,

		/* Broadcast events */

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
		LLMChatUnloaded,
		LLMGenerationStarted,
		LLMGenerationComplete,
		LLMCompletedMessage,
		LLMRebuildingKVCache,
		UserSignedIn,
		UserSignedOut,
		ChangedScreen,
		StartTextInput,
		StopTextInput,

		Count,
	};
	
	void RegisterUserEvents();
	extern uint32_t UserEventBase;

	inline uint32_t SDLUserEvent(UserEvent e) { return UserEventBase + static_cast<uint32_t>(e); }
	inline bool IsUserEvent(fig::event& event, UserEvent e) { return event.type == SDLUserEvent(e); }
	inline bool IsUserEventWithData(fig::event& event, UserEvent e) { return event.type == SDLUserEvent(e) and event.user.data1 != 0; }
	inline bool IsUserEvent(fig::event& event) { return static_cast<uint32_t>(event.type) >= UserEventBase && static_cast<uint32_t>(event.type) < UserEventBase + static_cast<uint32_t>(UserEvent::Count); }

	template <typename T>
	inline constexpr const T& GetUserData(fig::event& event) { return *reinterpret_cast<T*>(event.user.data1); }
	template <typename T>
	inline constexpr const T& GetUserData1(fig::event& event) { return *reinterpret_cast<T*>(event.user.data1); }
	template <typename T>
	inline constexpr const T& GetUserData2(fig::event& event) { return *reinterpret_cast<T*>(event.user.data2); }
	template <typename T, typename U>
	inline constexpr std::pair<const T*, const U*> GetUserData(fig::event& event) { return std::pair { reinterpret_cast<T*>(event.user.data1), reinterpret_cast<T*>(event.user.data2) }; }

	inline constexpr bool HasUserData(fig::event& event) { return (bool)event.user.data1; }
	inline constexpr bool HasUserData1(fig::event& event) { return (bool)event.user.data1; }
	inline constexpr bool HasUserData2(fig::event& event) { return (bool)event.user.data2; }

	extern bool IsBroadcastEvent(fig::event& event);

	inline void PushEvent(UserEvent eventType, int32_t code = 0)
	{
		fig::event event {};
		event.type = SDLUserEvent(eventType);
		event.user.code = code;
		SDL_PushEvent(&event);
	}

	template <typename T>
	inline void PushEvent(UserEvent eventType, T* pData)
	{
		fig::event event {};
		event.type = SDLUserEvent(eventType);
		event.user.data1 = (void*)pData;
		SDL_PushEvent(&event);
	}

	template <typename T, typename U>
	inline void PushEvent(UserEvent eventType, T* pData1, U* pData2)
	{
		fig::event event {};
		event.type = SDLUserEvent(eventType);
		event.user.data1 = (void*)pData1;
		event.user.data2 = (void*)pData2;
		SDL_PushEvent(&event);
	}

	template <typename T>
	inline void PushEvent(UserEvent eventType, int32_t code, T* pData)
	{
		fig::event event {};
		event.type = SDLUserEvent(eventType);
		event.user.code = code;
		event.user.data1 = (void*)pData;
		SDL_PushEvent(&event);
	}

	template <typename T, typename U>
	inline void PushEvent(UserEvent eventType, int32_t code, T* pData1, U* pData2)
	{
		fig::event event {};
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
