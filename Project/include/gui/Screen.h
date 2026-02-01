#ifndef SCREEN_H__
#define SCREEN_H__
#pragma once

#include "Control.h"

namespace fig::gui
{
	class Frame;

	enum class KeyModifier
	{
		None = 0,
		Control		= 1 << 0,
		Shift		= 1 << 1,
		Alt			= 1 << 2,
	};
	using KeyModifiers = EnumFlags<KeyModifier>;

	struct KeyboardEvent
	{
		SDL_Keycode key;
		KeyModifiers modifiers;
		bool pressed;
	};

	class Screen : public Control
	{
		friend class Frame;
	public:
		Screen(Frame* pParent);

	protected:
		bool OnEvent(Event& event) override;
		virtual bool OnKeyboardEvent(KeyboardEvent& event) = 0;
	};
}

#endif