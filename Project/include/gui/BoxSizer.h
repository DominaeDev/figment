#pragma once

#include "Sizer.h"

namespace fig::gui
{
	class BoxSizer : public Sizer
	{
		virtual fig::coord GetExtent() const = 0;

	protected:
		void OnLayout(const fig::rect& rect) override;

		virtual fig::coord GetAvailableSpace() = 0;
		virtual fig::coord GetItemSize(SizerItem& item, bool includeBorder) = 0;
		virtual std::pair<fig::coord, fig::coord> GetItemMinMaxSize(SizerItem& item) = 0;
		virtual fig::rect AllocateRect(fig::coord size) = 0;
		virtual void ExpandRect(fig::rect& rect, const fig::rect& allocated) = 0;

		fig::rect parentRect;
		fig::coord position;
	};
}