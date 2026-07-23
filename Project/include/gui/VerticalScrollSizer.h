#pragma once

#include "VerticalListSizer.h"

namespace fig::gui
{
	class VerticalScrollSizer : public VerticalListSizer
	{
	public:
		void SetOffset(fig::coord offset);

	protected:
		void OnLayout(const fig::rect& rect) override;

		fig::coord _offset = 0;
	};
}