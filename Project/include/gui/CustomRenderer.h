#pragma once

#include "Types.h"
#include "gui/GUITypes.h"

namespace fig::gui
{
	class CustomRenderer
	{
	public:
		CustomRenderer() = default;
		
		explicit CustomRenderer(Color color) :
			_color { color }
		{
		}

		virtual void Render(Renderer* pRenderer, const Rectf& rect) = 0;
		virtual ~CustomRenderer() = default;

		inline void SetColor(Color color) noexcept
		{
			_color = color;
		}

	protected:
		Color _color { Colors::White };
	};
}