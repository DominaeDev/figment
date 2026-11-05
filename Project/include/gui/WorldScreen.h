#pragma once

#include "gui/CardListScreen.h"

namespace fig::gui
{
	class WorldScreen : public CardListScreen<fig::data::World>
	{
	public:
		WorldScreen(Frame* pParent);

	protected:
		void ShowSortingMenu() noexcept override;
		void ShowFilteringMenu() noexcept override;
		std::pair<fig::io::FilterFlags, bool> GetFiltering() const noexcept override;
	};

	template <>
	constexpr ScreenType ScreenTypeOf<WorldScreen> = ScreenType::Worlds;
}
