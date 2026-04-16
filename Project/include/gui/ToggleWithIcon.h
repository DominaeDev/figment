#ifndef TOGGLE_WITH_ICON_H__
#define TOGGLE_WITH_ICON_H__
#pragma once

#include "gui/Control.h"
#include "gui/ThemedButton.h"

namespace fig::gui
{
	class TexturedBorderRenderer;

	using ToggleDelegate = std::function<void(bool)>;

	class ToggleWithIcon : public ThemedButton
	{
		ToggleWithIcon() = delete;
	public:
		enum class ToggleBehavior { Default, Radio };
		ToggleWithIcon(LayoutElement* pParent, TextureType icon, ToggleBehavior behavior = ToggleBehavior::Default, bool bOn = false);

		void SetIcon(TextureType icon);
		void Toggle(bool bOn, bool bTrigger = true) noexcept;

		void SetDelegate(ToggleDelegate pDelegate) noexcept;

	protected:
		void OnSize() override;
		void OnButtonState() override;

	private:
		TexturedBorderRenderer* _pBGRenderer;
		ToggleDelegate _fnToggle {};
		Image* _pIcon {};
		bool _bOn {};
		ToggleBehavior _behavior {};
	};
}

#endif