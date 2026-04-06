#pragma once

#include "Types.h"
#include "CustomRenderer.h"

namespace fig::gui
{
	class SolidFillRenderer : public CustomRenderer
	{
	public:
		explicit SolidFillRenderer(Color color = Colors::White);

		void Render(Renderer* pRenderer, const Rectf& rect);
	};
}