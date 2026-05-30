#ifndef SCREEN_H__
#define SCREEN_H__
#pragma once

#include "Control.h"
#include "gui/KeyboardMods.h"

namespace fig::gui
{
	class Frame;

	struct KeyboardEvent
	{
		SDL_Keycode key;
		KeyboardMods modifiers;
		bool pressed;
	};

	class Screen : public Control
	{
		friend class Frame;
	public:
		Screen(Frame* pParent);

		void NotifySidePanelShown(bool showing);
		
		void PushEvent(UserEvent eventType, int32_t code = 0, void* pData1 = nullptr, void* pData2 = nullptr);
	protected:
		EventResult OnEvent(Event& event) override;
		virtual void OnSidePanel(bool show) {};
		virtual bool OnKeyboardEvent(KeyboardEvent& event) = 0;

		virtual void OnUserSignedIn(const fig::user::UserProfile& profile) {};
		virtual void OnUserSignedOut() {};
	};

	template <typename T>
	concept IsScreen = std::derived_from<T, fig::gui::Screen>;
}

#endif