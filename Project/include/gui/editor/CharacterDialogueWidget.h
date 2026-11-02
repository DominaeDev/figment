#pragma once

#include "gui/Control.h"
#include "gui/MouseEventHandler.h"
#include "data/Character.h"

namespace fig::gui
{
	class CharacterDialogueWidget : public Control, public MouseEventHandler, public ITabControl
	{
	public:
		CharacterDialogueWidget(ControlPtr pParent, const fig::data::ExampleDialogue& storyEvent, fig::observer_ptr<const fig::data::Character> pCharacter);

		using ValueChangedDelegate = std::function<void(fig::data::ExampleDialogue)>;
		using MoveDelegate = std::function<void(int32_t)>;
		void SetValueChangedDelegate(ValueChangedDelegate fnDelegate);
		void SetButtonDelegate(MouseClickedDelegate fnDelegate);
		void SetMoveDelegate(MoveDelegate fnDelegate);

		fig::data::ExampleDialogue GetValue() const noexcept;

		void SetFocus(bool bFocus, TabDirection dir = TabDirectionForward);
		bool CanFocus() const { return true; }
		bool IsFocused() const;

		fig::point GetPreferredSize() const noexcept override;

	protected:
		void OnUpdate(float) override;
		void OnSize() override;
		EventResult OnEvent(fig::event& storyEvent) override;

		void OnMove(int32_t dir) noexcept;
		void OnValueChanged() noexcept;

	private:
		fig::observer_ptr<const fig::data::Character> _pCharacter;

		fig::observer_ptr<StaticText> _pInputLabel;
		fig::observer_ptr<StaticText> _pResponseLabel;
		fig::observer_ptr<TextBox> _pInput;
		fig::observer_ptr<TextBox> _pResponse;
		fig::observer_ptr<ButtonWithIcon> _pSettingsButton;
		fig::observer_ptr<ButtonWithIcon> _pMoveUpButton;
		fig::observer_ptr<ButtonWithIcon> _pMoveDownButton;

		fig::coord _lastTextBoxHeight = 0uz;

		MoveDelegate _fnMoveDelegate;
		ValueChangedDelegate _fnChangedDelegate;
	};
}