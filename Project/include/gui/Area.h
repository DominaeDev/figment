#pragma once

#include "Control.h"

/// <summary>
/// Render-less panel
/// </summary>

namespace fig::gui
{
	class Area : public Control
	{
	public:
		Area(control_ptr pParent);

	protected:
		void OnUpdate(float fElapsed) override {}
		void OnRender(fig::renderer_ptr pRenderer) override {}
	};
}