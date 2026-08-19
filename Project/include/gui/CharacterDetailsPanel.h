#pragma once

#include "gui/ScrollPanel.h"
#include "data/Character.h"

namespace fig::gui
{
	class CharacterDetailsPanel : public ScrollPanel
	{
	public:
		CharacterDetailsPanel(ControlPtr pParent);

		void SetCharacter(const fig::data::Character& character);
	
	protected:
		fig::coord GetExtent() const override;

	private:
		fig::observer_ptr<StaticText> _pHeader;
		fig::observer_ptr<StaticText> _pDescription;
	};
}
