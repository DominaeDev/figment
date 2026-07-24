#pragma once

#include "Figment.h"
#include "CustomRenderer.h"

namespace fig::gui
{
	class LineBorderRenderer : public CustomRenderer
	{
	public:
		explicit LineBorderRenderer(fig::color color = Color::White, Directions directions = { Direction::North, Direction::East, Direction::South, Direction::West });
		LineBorderRenderer(fig::color color, Direction direction);

		void Render(fig::renderer_ptr pRenderer, const fig::rectf& rect);

	private:
		Directions _directions {};
	};
}