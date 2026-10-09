#pragma once

#include "gui/Screen.h"
#include "gui/GUICommon.h"

namespace fig::gui
{
	class DebugScreen : public Screen
	{
	public:
		DebugScreen(Frame* pParent);

	protected:
		void OnActivated() override;
		void OnUpdate(float fElapsed) override;
		void OnRender(fig::renderer_ptr pRenderer) override;
		bool OnKeyboardEvent(KeyboardEvent& event) override;

	private:
		fig::observer_ptr<class ImageCarousel> _pCarousel;
	};

	template <>
	constexpr ScreenType ScreenTypeOf<DebugScreen> = ScreenType::Debug;
}
