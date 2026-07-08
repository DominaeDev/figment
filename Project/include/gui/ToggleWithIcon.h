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
		ToggleWithIcon(ParentPtr pParent, TextureType icon, ToggleBehavior behavior = ToggleBehavior::Default, bool bOn = false);

		void SetIcon(TextureType icon);
		void Toggle(bool bOn, bool bTrigger = true) noexcept;

		void SetDelegate(ToggleDelegate pDelegate) noexcept;

	protected:
		void OnSize() override;
		void OnButtonState() override;

	private:
		fig::observer_ptr<Image> _pIcon {};
		ToggleDelegate _fnToggle {};
		ToggleBehavior _behavior {};
		bool _bOn {};
	};
}
