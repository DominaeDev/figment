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
		Area(ParentPtr pParent);

	protected:
		void OnUpdate(float fElapsed) override {}
		void OnRender(Renderer* pRenderer) override {}
	};
}