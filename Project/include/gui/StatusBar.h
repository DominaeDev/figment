#pragma once

#include "Control.h"
#include "Types.h"

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
		StatusBar(Control* pParent);

		void SetMessage(fig::string_view message);
		void SetModelInfo(fig::llm::LLMStatus status);

	private:
		StaticText* _pMessage;
		StaticText* _pModelInfo;
	};
}