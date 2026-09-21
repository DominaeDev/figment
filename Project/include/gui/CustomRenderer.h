#pragma once

#include "Figment.h"
#include "gui/GUITypes.h"

namespace fig::gui
{
	class CustomRenderer
	{
	public:
		CustomRenderer() = default;
		
		explicit CustomRenderer(fig::color_ref color) :
			_color { color }
		{
		}

		virtual void Render(fig::renderer_ptr pRenderer, const fig::rectf& rect) = 0;
		virtual ~CustomRenderer() = default;

		void SetColor(fig::color_ref color) noexcept {
			_color = color;
		}

	protected:
		fig::color_ref _color { Colour::White };
	};
}