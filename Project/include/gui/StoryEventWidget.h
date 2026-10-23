#pragma once

#include "gui/Control.h"
#include "gui/MouseEventHandler.h"
#include "data/Scenario.h"
#include "DropList.h"

namespace fig::gui
{
	class TextBox;
	class TextInput;
	class ButtonWithIcon;
	class ComboBox;

	class StoryEventWidget : public Control, public MouseEventHandler, public ITabControl
	{
	public:
		StoryEventWidget(ControlPtr pParent, const fig::data::Scenario::Event& storyEvent, fig::observer_ptr<const fig::data::Scenario> pScenario);

		using ValueChangedDelegate = std::function<void(fig::data::Scenario::Event)>;
		using MoveDelegate = std::function<void(int32_t)>;
		void SetValueChangedDelegate(ValueChangedDelegate fnDelegate);
		void SetButtonDelegate(MouseClickedDelegate fnDelegate);
		void SetMoveDelegate(MoveDelegate fnDelegate);

		fig::chat::Role GetRole() const noexcept;
		fig::string_view GetContent() const noexcept;
		fig::data::Scenario::Event GetValue() const noexcept;

		void SetFocus(bool bFocused);
		bool CanFocus() const { return true; }
		bool IsFocused() const;

	protected:
		void CreateRoleDropList(fig::chat::Role role);
		void CreateContentTextBox(fig::string_view text, fig::string_view placeholder, bool bMultiline);
		void InitValue(const fig::data::Scenario::Event& storyEvent);
		void OnUpdate(float) override;
		void OnSize() override;
		EventResult OnEvent(fig::event& storyEvent) override;

		void OnMove(int32_t dir) noexcept;
		void OnValueChanged() noexcept;

	private:
		fig::observer_ptr<const fig::data::Scenario> _pScenario;

		fig::data::Scenario::Event::Type _eventType {};

		fig::observer_ptr<StaticText> _pLabel;
		fig::observer_ptr<TextBox> _pTextBox;
		fig::observer_ptr<DropListOfType<fig::chat::Role>> _pDropList;
		fig::observer_ptr<ButtonWithIcon> _pSettingsButton;
		fig::observer_ptr<ButtonWithIcon> _pMoveUpButton;
		fig::observer_ptr<ButtonWithIcon> _pMoveDownButton;

		fig::coord _lastTextBoxHeight = 0uz;

		MoveDelegate _fnMoveDelegate;
		ValueChangedDelegate _fnChangedDelegate;
	};
}