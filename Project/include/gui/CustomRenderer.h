#pragma once

#include "Types.h"
#include "gui/GUITypes.h"

namespace fig::gui
{
	class CustomRenderer
	{
	public:
		virtual void Render(Renderer* pRenderer, const Rectf& rect) = 0;
		virtual ~CustomRenderer() = default;
	};
}