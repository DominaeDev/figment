#pragma once

#include "gui/Control.h"
#include "Figment.h"

namespace fig::llm
{
	struct LLMStatus;
}

namespace fig::gui
{
	class StaticText;

	class StatusBar : public Control
	{
	public:
		StatusBar(ControlPtr pParent);

		void SetMessage(const fig::string& message);
		void SetModelInfo(const fig::llm::LLMStatus& status);

	protected:
		EventResult OnEvent(fig::event& event) override;
	private:
		fig::observer_ptr<StaticText> _pMessage;
		fig::observer_ptr<StaticText> _pModelInfo;
	};
}