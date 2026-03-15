#pragma once

#include "VerticalListSizer.h"

namespace fig::gui
{
	class VerticalScrollSizer : public VerticalListSizer
	{
	public:
		void SetOffset(Coord offset);

	protected:
		void OnLayout(Rect rect) override;

		Coord _offset = 0;
	};
}