#pragma once

#include "gui/ModalOverlay.h"

namespace fig::gui
{
	enum class DialogButton
	{
		Ok = 1 << 0,
		Yes = 1 << 1,
		Confirm = 1 << 2,
		No = 1 << 3,
		Dismiss = 1 << 4,
		Cancel = 1 << 5,
	};
	using DialogButtons = EnumFlags<DialogButton>;

	using DialogBoxDelegate = std::function<void(DialogButton)>;

	class DialogBox : public ModalOverlay
	{
	public:
		DialogBox(FramePtr pFrame, fig::string_view message, DialogButtons buttons, DialogBoxDelegate fnDelegate);

		void SetText(fig::string_view text);

	protected:
		void OnUpdate(float fElapsed) override;
		void OnRender(fig::renderer_ptr pRenderer) override;
		void OnSize() override;
		EventResult OnEvent(fig::event& event) override;
		void OnButton(DialogButton button);

		void CreateButton(SizerPtr pSizer, DialogButton button, fig::string_view label);

		fig::observer_ptr<Panel> _pBox;
		fig::observer_ptr<StaticText> _pMessage;
		DialogBoxDelegate _fnDelegate;
	};
}