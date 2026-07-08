#pragma once

#include "gui/Control.h"
#include "gui/GUITypes.h"
#include <map>

namespace fig::gui
{
	class StaticText;

	class VariableList : public Control
	{
	public:
		VariableList(ParentPtr pParent);
		void SetVariables(const std::map<fig::string, fig::string>& variables);
		bool IsEmpty() const;

	protected:
		void OnRender(Renderer* pRenderer) override;

	private:
		fig::observer_ptr<StaticText> _pText;
	};
}