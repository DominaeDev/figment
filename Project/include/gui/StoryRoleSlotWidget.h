#pragma once

#include "gui/Control.h"
#include "data/Scenario.h"
#include "DropList.h"

namespace fig::gui
{
	class TextBox;
	class TextInput;
	class ButtonWithIcon;
	class ComboBox;

	class StoryRoleSlotWidget : public Control, public ITabControl
	{
	public:
		StoryRoleSlotWidget(ControlPtr pParent, fig::observer_ptr<fig::data::Scenario> pScenario, fig::chat::Role role);


		void SetFocus(bool bFocus, TabDirection dir = TabDirectionForward) override;
		bool CanFocus() const override;
		bool IsFocused() const override;

	protected:
		void InitValue();
		bool RefreshHeight();
		void OnUpdate(float) override;
		void OnSize() override;

		fig::observer_ptr<StaticText> CreateLabel(SizerPtr pSizer, fig::string_view text);
		fig::observer_ptr<TextBox> CreateTextBox(SizerPtr pSizer, fig::string_view text, fig::string_view placeholder, TextInput::Mode mode);
		fig::observer_ptr<fig::gui::StaticText> CreateHint(SizerPtr pSizer, fig::string_view text);
		void OnChangeName(fig::string_view value);
		void OnChangeHandle(fig::string_view value);
		void OnChangeBrief(fig::string_view value);
		void OnChangePrompt(fig::string_view value);
		void OnChangeCondition(fig::string_view value);

	private:
		fig::observer_ptr<fig::data::Scenario> _pScenario;
		fig::chat::Role _role;

		fig::observer_ptr<TextBox> _pName;
		fig::observer_ptr<TextBox> _pHandle;
		fig::observer_ptr<TextBox> _pBrief;
		fig::observer_ptr<TextBox> _pPrompt;
		fig::observer_ptr<TextBox> _pCondition;
		fig::observer_ptr<VerticalBar> _pColorBar;

		fig::coord _lastTextBoxHeight = 0uz;
	};
}