#pragma once

#include "gui/Control.h"
#include "data/Character.h"

namespace fig::gui
{
	class TextBox;
	class TextInput;
	class ButtonWithIcon;
	class ComboBox;

	class CharacterAttributeWidget : public Control, public MouseEventHandler
	{
	public:
		CharacterAttributeWidget(ControlPtr pParent, fig::string_view label, fig::string_view content, fig::data::CharacterAttribute::ValueType type, const fig::string_list& options = {}, fig::string_view placeholder = {});

		using MoveDelegate = std::function<void(int32_t)>;
		void SetMoveDelegate(MoveDelegate fnDelegate);
		void SetButtonDelegate(MouseClickedDelegate fnDelegate);

		fig::string_view GetValue() const noexcept;
		void Focus();
		void EnableRename(bool bEnable) noexcept;

		void SetLabel(fig::string_view label);

		void ChangeType(fig::data::CharacterAttribute::ValueType type);

		using EditNameDelegate = std::function<void(fig::string)>;
		void SetEditNameDelegate(EditNameDelegate fnDelegate);
		
		void BeginEditName();

	protected:
		void InitValue(fig::string_view value, fig::data::CharacterAttribute::ValueType type);
		void OnUpdate(float) override;
		void OnSize() override;
		EventResult OnEvent(fig::event& event) override;

		void EndEditName();
		void CancelEditName();
		void OnDoubleClickedAt(fig::point pos) override;
		void OnMove(int32_t dir) noexcept;

	private:
		fig::observer_ptr<StaticText> _pLabel;
		fig::observer_ptr<TextInput> _pEditLabel;
		fig::observer_ptr<TextBox> _pTextBox;
		fig::observer_ptr<ComboBox> _pComboBox;
		fig::observer_ptr<ButtonWithIcon> _pSettingsButton;
		fig::observer_ptr<ButtonWithIcon> _pMoveUpButton;
		fig::observer_ptr<ButtonWithIcon> _pMoveDownButton;

		fig::coord _lastTextBoxHeight = 0uz;
		fig::string_list _options {};

		bool _bCanRename { false };
		bool _bRenaming {};
		EditNameDelegate _fnRenameDelegate;

		MoveDelegate _fnMoveDelegate;
	};
}