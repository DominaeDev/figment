#pragma once

#include "gui/IMeshControl.h"

namespace fig::gui
{
	class VerticalBar : public IMeshControl
	{
	public:
		VerticalBar(ControlPtr pParent, Resource texture, float fCapSizeV = 0.25f);

	protected:
		void RefreshGeometry(const fig::rectf& rect) override;

	private:
		float _fCapSizeV = 0.25f;
	};
}