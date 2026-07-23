#pragma once

#include "Figment.h"

namespace fig::gui
{
	enum class KeyModifier
	{
		None = 0,
		Control = 1 << 0,
		Shift = 1 << 1,
		Alt = 1 << 2,
	};
	using KeyModifiers = EnumFlags<KeyModifier>;

	struct KeyboardMods
	{
		KeyboardMods() = delete;
		constexpr explicit KeyboardMods(const fig::event& event) : 
			None((event.key.mod & (SDL_KMOD_CTRL | SDL_KMOD_SHIFT | SDL_KMOD_ALT)) == 0),
			ControlDown((event.key.mod & SDL_KMOD_CTRL) != 0),
			ShiftDown((event.key.mod & SDL_KMOD_SHIFT) != 0),
			AltDown((event.key.mod & SDL_KMOD_ALT) != 0),
			Control((event.key.mod & SDL_KMOD_CTRL) && !(event.key.mod & SDL_KMOD_SHIFT) && !(event.key.mod & SDL_KMOD_ALT)),
			Shift((event.key.mod & SDL_KMOD_SHIFT) && !(event.key.mod & SDL_KMOD_CTRL) && !(event.key.mod & SDL_KMOD_ALT)),
			Alt((event.key.mod & SDL_KMOD_ALT) && !(event.key.mod & SDL_KMOD_SHIFT) && !(event.key.mod & SDL_KMOD_CTRL)),
			ControlAlt((event.key.mod & SDL_KMOD_CTRL) && !(event.key.mod & SDL_KMOD_SHIFT) && !(event.key.mod & SDL_KMOD_ALT)),
			ShiftAlt(!(event.key.mod & SDL_KMOD_CTRL) && (event.key.mod & SDL_KMOD_SHIFT) && (event.key.mod & SDL_KMOD_ALT)),
			ControlShift((event.key.mod & SDL_KMOD_CTRL) && (event.key.mod & SDL_KMOD_SHIFT) && !(event.key.mod & SDL_KMOD_ALT)),
			ControlShiftAlt((event.key.mod & SDL_KMOD_CTRL) && (event.key.mod & SDL_KMOD_SHIFT) && (event.key.mod & SDL_KMOD_ALT))
		{
		}

		constexpr explicit KeyboardMods(const SDL_KeyboardEvent& event) :
			None((event.mod & (SDL_KMOD_CTRL | SDL_KMOD_SHIFT | SDL_KMOD_ALT)) == 0),
			ControlDown((event.mod & SDL_KMOD_CTRL) != 0),
			ShiftDown((event.mod & SDL_KMOD_SHIFT) != 0),
			AltDown((event.mod & SDL_KMOD_ALT) != 0),
			Control((event.mod & SDL_KMOD_CTRL) && !(event.mod & SDL_KMOD_SHIFT) && !(event.mod & SDL_KMOD_ALT)),
			Shift((event.mod & SDL_KMOD_SHIFT) && !(event.mod & SDL_KMOD_CTRL) && !(event.mod & SDL_KMOD_ALT)),
			Alt((event.mod & SDL_KMOD_ALT) && !(event.mod & SDL_KMOD_SHIFT) && !(event.mod & SDL_KMOD_CTRL)),
			ControlAlt((event.mod & SDL_KMOD_CTRL) && !(event.mod & SDL_KMOD_SHIFT) && !(event.mod & SDL_KMOD_ALT)),
			ShiftAlt(!(event.mod & SDL_KMOD_CTRL) && (event.mod & SDL_KMOD_SHIFT) && (event.mod & SDL_KMOD_ALT)),
			ControlShift((event.mod & SDL_KMOD_CTRL) && (event.mod & SDL_KMOD_SHIFT) && !(event.mod & SDL_KMOD_ALT)),
			ControlShiftAlt((event.mod & SDL_KMOD_CTRL) && (event.mod & SDL_KMOD_SHIFT) && (event.mod & SDL_KMOD_ALT))
		{
		}

		const bool None;
		const bool ControlDown;
		const bool ShiftDown;
		const bool AltDown;
		const bool Control;
		const bool Shift;
		const bool Alt;
		const bool ControlAlt;
		const bool ShiftAlt;
		const bool ControlShift;
		const bool ControlShiftAlt;
	};
}
