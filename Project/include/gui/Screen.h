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
		
		void PushEvent(UserEvent eventType, int32_t code = 0, void* pData1 = nullptr, void* pData2 = nullptr);
	protected:
		EventResult OnEvent(fig::event& event) override;
		virtual bool OnKeyboardEvent(KeyboardEvent& event) = 0;

		virtual void OnUserSignedIn(const fig::user::UserProfile& profile) {};
		virtual void OnUserSignedOut() {};
	};

	template <typename T>
	concept IsScreen = std::derived_from<T, Screen>;

	enum class ScreenType : size_t
	{
		Undefined,
		Debug,
		Login,
		Home,
		Chat,
		ChatListing,
		Editor,
	};

	template <IsScreen T>
	inline constexpr ScreenType ScreenTypeOf = []<bool Flag = false>()
	{
		static_assert(Flag, "No ScreenType mapping for this type");
	}();

}
