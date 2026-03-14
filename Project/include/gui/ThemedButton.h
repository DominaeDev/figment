#ifndef THEMED_BUTTON_BASE_H__
#define THEMED_BUTTON_BASE_H__
#pragma once

#include "Control.h"
#include "gui/BaseButton.h"

namespace fig::gui
{
	class ThemedButton : public Control, public BaseButton
	{
		ThemedButton() = delete;
	public:
		void SetTheme(const ButtonTheme& theme) noexcept;

	protected:
		ThemedButton(LayoutElement* pParent);

		const Color& GetThemeForeground() const noexcept;
		const Color& GetThemeBackground() const noexcept;

		bool OnEvent(Event& event) override;
		void OnAfterLayout() override;

	private:
		ButtonTheme _theme {};
	};
}

#endif