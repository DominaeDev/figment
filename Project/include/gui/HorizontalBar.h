#pragma once

#include "gui/IMeshControl.h"

namespace fig::gui
{
	class HorizontalBar : public IMeshControl
	{
	public:
		HorizontalBar(ControlPtr pParent, Resource texture, float fCapSizeU = 0.25f);

	protected:
		void RefreshGeometry(const fig::rectf& rect) override;

	private:
		float _fCapSizeU = 0.25f;
	};
}