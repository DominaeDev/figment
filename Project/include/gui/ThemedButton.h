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

		fig::color_ref GetThemeForeground() const noexcept;
		fig::color_ref GetThemeBackground() const noexcept;
		fig::color_ref GetThemeBorderColor() const noexcept;

		void OnAfterLayout() override;
		EventResult OnEvent(fig::event& event) override;
		void OnEnabled(bool bEnabled) override;

	protected:
		ButtonTheme _theme {};
	};
}
