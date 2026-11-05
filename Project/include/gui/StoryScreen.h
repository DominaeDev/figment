#pragma once

#include "gui/CardListScreen.h"

namespace fig::gui
{
	class StoryScreen : public CardListScreen<fig::data::Scenario>
	{
	public:
		StoryScreen(Frame* pParent);

	protected:
		void ShowSortingMenu() noexcept override;
		void ShowFilteringMenu() noexcept override;
		std::pair<fig::io::FilterFlags, bool> GetFiltering() const noexcept override;
	};

	template <>
	constexpr ScreenType ScreenTypeOf<StoryScreen> = ScreenType::Stories;
}
