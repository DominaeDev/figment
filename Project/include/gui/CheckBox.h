#pragma once

#include "gui/Control.h"
#include "gui/MouseEventHandler.h"

namespace fig::gui
{
	using OnCheckedDelegate = std::function<void(bool)>;

	class CheckBox : public Control, public MouseEventHandler
	{
	public:
		CheckBox(ControlPtr pParent, fig::string_view label = "", bool bOn = false);

		bool GetValue() const noexcept { return _bOn; }
		void SetValue(bool bOn, bool bSilent = false) noexcept;
		void SetLabel(fig::string_view) noexcept;
		void SetDelegate(OnCheckedDelegate fnDelegate);

	protected:
		EventResult OnEvent(fig::event& event) override;
		void OnSize() override;
		void OnClicked() override;
		void OnButtonState() override;
		void OnEnabled(bool bEnabled) override;

	private:
		bool _bOn {};
		OnCheckedDelegate _fnDelegate;

		fig::observer_ptr<Panel> _pBox;
		fig::observer_ptr<Image> _pTick;
		fig::observer_ptr<StaticText> _pLabel;
	};
}