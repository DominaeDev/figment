#pragma once

#include "gui/Control.h"
#include "gui/MouseEventHandler.h"

namespace fig::gui
{
	class ThemedButton : public Control, public MouseEventHandler
	{
		ThemedButton() = delete;
	public:
		void SetTheme(const ButtonTheme& theme) noexcept;
		const ButtonTheme& GetTheme() const noexcept { return _theme; }

	protected:
		ThemedButton(ControlPtr pParent);

		const fig::color& GetThemeForeground() const noexcept;
		const fig::color& GetThemeBackground() const noexcept;

		void OnAfterLayout() override;
		EventResult OnEvent(fig::event& event) override;
		void OnEnabled(bool bEnabled) override;

	protected:
		ButtonTheme _theme {};
	};
}
