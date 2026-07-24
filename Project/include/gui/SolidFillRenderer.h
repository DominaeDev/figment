#pragma once

#include "Figment.h"
#include "CustomRenderer.h"

namespace fig::gui
{
	class SolidFillRenderer : public CustomRenderer
	{
	public:
		explicit SolidFillRenderer(fig::color color = Color::White);

		void Render(fig::renderer_ptr pRenderer, const fig::rectf& rect);
	};
}