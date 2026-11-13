#pragma once

#include "gui/ModalOverlay.h"

namespace fig::gui
{
	using TextInputDialogDelegate = std::function<void(fig::string_view)>;

	class TextInputDialogBox : public ModalOverlay
	{
	public:
		TextInputDialogBox(FramePtr pFrame, fig::string_view message, TextInputDialogDelegate fnDelegate);

		void SetDefaultValue(fig::string_view text);
		void SetPlaceholder(fig::string_view text);
		void SetMaxLength(size_t length);

	protected:
		void SetMessage(fig::string_view text);
		void OnUpdate(float fElapsed) override;
		void OnSize() override;
		EventResult OnEvent(fig::event& event) override;
		fig::observer_ptr<ButtonWithLabel> CreateButton(SizerPtr pSizer, fig::string_view label);
		void OnResult(bool bConfirm);

		fig::observer_ptr<Panel> _pBox;
		fig::observer_ptr<TextInput> _pInput;
		fig::observer_ptr<StaticText> _pMessage;
		fig::observer_ptr<ThemedButton> _pButtonOk;
		TextInputDialogDelegate _fnDelegate;
	};
}