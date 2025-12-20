#pragma once

#include "VerticalListSizer.h"

namespace fig::gui
{
	class VerticalScrollSizer : public VerticalListSizer
	{
	public:
		void SetOffset(float offset);

	protected:
		void OnLayout(Rectf rect) override;

		float _offset = 0.0f;
	};
}