#pragma once

#include "gui/BoxSizer.h"

namespace fig::gui
{
	class VerticalSizer : public BoxSizer
	{
	protected:
		fig::coord GetAvailableSpace() override;
		fig::coord GetItemSize(SizerItem& item, bool includeBorder) override;
		std::pair<fig::coord, fig::coord> GetItemMinMaxSize(SizerItem& item) override;
		fig::rect AllocateRect(fig::coord size) override;
		void ExpandRect(fig::rect& rect, const fig::rect& allocated) override;
	};
}