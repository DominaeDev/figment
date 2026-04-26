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
		StatusBar(LayoutElement* pParent);

		void SetMessage(const fig::string& message);
		void SetModelInfo(const fig::llm::LLMStatus& status);

	private:
		StaticText* _pMessage;
		StaticText* _pModelInfo;
	};
}