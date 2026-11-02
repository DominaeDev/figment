#pragma once

#include "util/ObserverPtr.h"

namespace fig::gui
{
	using TabDirection = int32_t;
	constexpr TabDirection TabDirectionForward = 1;
	constexpr TabDirection TabDirectionBackward = -1;
	class ITabControl
	{
	public:
		virtual void SetFocus(bool, TabDirection) = 0;
		virtual bool IsFocused() const = 0;
		virtual bool CanFocus() const = 0;
		virtual bool FocusNext() { return false; }
		virtual bool FocusPrev() { return false; }

	protected:
		using TabControlPtr = fig::observer_ptr<ITabControl>;

		template<typename... TControls>
		static std::optional<int32_t> FindFocused(fig::observer_ptr<TControls>... pControls)
		{
			TabControlPtr const controls[] = { pControls... };
			constexpr int32_t controlCount = sizeof...(pControls);

			for (int32_t index = 0; index < controlCount; index++)
			{
				if (controls[index]->IsFocused())
					return index;
			}

			return std::nullopt;
		}

		template<typename... TControls>
		static bool FocusAdjacent(int32_t step, TabDirection direction, fig::observer_ptr<TControls>... pControls)
		{
			TabControlPtr const controls[] = { pControls... };
			int32_t controlCount = sizeof...(pControls);
			std::optional<int32_t> current = FindFocused(pControls...);

			int32_t index = step > 0 ? 0 : controlCount - 1;
			if (current)
				index = *current + step;

			for (; index >= 0 && index < controlCount; index += step)
			{
				if (!controls[index]->CanFocus())
					continue;

				if (current)
					controls[*current]->SetFocus(false, direction);

				controls[index]->SetFocus(true, direction);
				return true;
			}

			return false;
		}

		template<typename... TControls>
		bool FocusNext(fig::observer_ptr<TControls>... pControls)
		{
			return FocusAdjacent(1, TabDirectionForward, pControls...);
		}

		template<typename... TControls>
		bool FocusPrev(fig::observer_ptr<TControls>... pControls)
		{
			return FocusAdjacent(-1, TabDirectionBackward, pControls...);
		}
	};

	class TabControlGroup;
}