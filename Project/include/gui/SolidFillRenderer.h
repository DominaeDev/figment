#pragma once

#include "Figment.h"
#include "CustomRenderer.h"

namespace fig::gui
{
	class SolidFillRenderer : public CustomRenderer
	{
	public:
		explicit SolidFillRenderer(fig::color color = Colors::White);

		void Render(fig::renderer_ptr pRenderer, const fig::rectf& rect);
	};
}