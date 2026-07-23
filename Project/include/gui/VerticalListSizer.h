#pragma once

#include "Sizer.h"

namespace fig::gui
{
	class VerticalListSizer : public Sizer
	{
	public:
		void SetSpacing(int spacing) { _spacing = spacing; }
		void SetBottomMargin(int margin) { _marginBottom = margin; }

		float GetListHeight() const { return _totalListHeight; }

	protected:
		void OnLayout(const fig::rect& rect) override;

		float _totalListHeight {};
		int _marginBottom {};
		int _spacing {};
	};
}