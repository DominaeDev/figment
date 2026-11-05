#pragma once

#include "gui/CardListScreen.h"

namespace fig::gui
{
	class CharacterScreen : public CardListScreen<fig::data::Character>
	{
	public:
		CharacterScreen(Frame* pParent);

	protected:
		EventResult OnEvent(fig::event& event) override;

		void ShowSortingMenu() noexcept override;
		void ShowFilteringMenu() noexcept override;

		std::pair<fig::io::FilterFlags, bool> GetFiltering() const noexcept override;
	};

	template <>
	constexpr ScreenType ScreenTypeOf<CharacterScreen> = ScreenType::Home;
}
