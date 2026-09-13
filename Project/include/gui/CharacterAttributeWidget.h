#pragma once

#include "gui/Control.h"
#include "data/Character.h"

namespace fig::gui
{
	class TextBox;
	class TextInput;
	class ButtonWithIcon;

	class CharacterAttributeWidget : public Control
	{
	public:
		CharacterAttributeWidget(ControlPtr pParent, fig::data::CharacterAttribute& attribute);

	protected:
		void InitValue();
		void ShowMenu();

		void OnUpdate(float) override;
		void OnSize() override;

	private:
		fig::observer_ptr<fig::data::CharacterAttribute> _pAttribute {};
		fig::observer_ptr<StaticText> _pLabel;
		fig::observer_ptr<TextInput> _pEditLabel;
		fig::observer_ptr<TextBox> _pTextBox;
		fig::observer_ptr<ButtonWithIcon> _pSettingsButton;

		fig::coord _lastTextBoxHeight = 0uz;
	};
}