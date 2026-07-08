#pragma once

#include "Figment.h"
#include "CustomRenderer.h"

namespace fig::gui
{
	enum class Direction
	{
		North = 1 << 0,
		East = 1 << 1,
		South = 1 << 2,
		West = 1 << 3,
	};
	using Directions = EnumFlags<Direction>;

	class LineBorderRenderer : public CustomRenderer
	{
	public:
		explicit LineBorderRenderer(Color color = Colors::White, Directions directions = { Direction::North, Direction::East, Direction::South, Direction::West });
		LineBorderRenderer(Color color, Direction direction);

		void Render(Renderer* pRenderer, const Rectf& rect);

	private:
		Directions _directions {};
	};
}