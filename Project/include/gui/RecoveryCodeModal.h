#pragma once

#include "gui/ModalOverlay.h"

namespace fig::gui
{
	class RecoveryCodeModal : public ModalOverlay
	{
	public:
		RecoveryCodeModal(FramePtr pFrame, fig::string_view code);

	protected:
		void OnSize() override;
		EventResult OnEvent(fig::event& event) override;


	private:
		void CreateCodeRow(ControlPtr pParent, SizerPtr pSizer, fig::string_view code, size_t index);

		fig::observer_ptr<Panel> _pBox;
		fig::observer_ptr<StaticText> _pMessage;
	};
}