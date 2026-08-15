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
		const ButtonTheme& GetTheme() const noexcept { return _theme; }

	protected:
		ThemedButton(ControlPtr pParent);

		const fig::color& GetThemeForeground() const noexcept;
		const fig::color& GetThemeBackground() const noexcept;

		EventResult OnEvent(fig::event& event) override;
		void OnAfterLayout() override;

	protected:
		ButtonTheme _theme {};
	};
}
