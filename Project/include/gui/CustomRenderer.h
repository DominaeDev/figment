#pragma once

#include "Figment.h"
#include "gui/GUITypes.h"

namespace fig::gui
{
	class CustomRenderer
	{
	public:
		CustomRenderer() = default;
		
		explicit CustomRenderer(fig::color color) :
			_color { color }
		{
		}

		virtual void Render(fig::renderer_ptr pRenderer, const fig::rectf& rect) = 0;
		virtual ~CustomRenderer() = default;

		inline void SetColor(fig::color color) noexcept
		{
			_color = color;
		}

	protected:
		fig::color _color { Colors::White };
	};
}