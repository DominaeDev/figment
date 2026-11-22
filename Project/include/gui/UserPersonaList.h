#pragma once

#include "gui/ScrollPanel.h"
#include "gui/UserPersonaListItem.h"
#include "user/UserSettings.h"

namespace fig::gui
{
	class GridSizer;
	class Menu;
	class MenuItem;

	class UserPersonaList : public ScrollPanel
	{
	public:
		UserPersonaList(ControlPtr pParent);

		void CreateCards();
		void RefreshCards();
		void Clear();

		void SetFilter(const fig::string& filter) noexcept;
		void Reorder();

	protected:
		void OnUpdate(float fElapsed) override;
		void OnScroll() override;

		fig::coord GetExtent() const override;
		std::tuple<fig::io::SortBy, fig::io::OrderBy, fig::io::FilterFlags> GetSortOrder();
		void OnItemEvent(UserPersonaListItem& item, UserPersonaListItemEvent event);

		void DeleteUserPersona(UserPersonaListItem& item);

		bool _bInitialized { false };
		std::vector<fig::observer_ptr<UserPersonaListItem>> _items;

		fig::observer_ptr<GridSizer> _pGridSizer;
		int32_t _last_rows {};
		fig::string _filterString;
	};
}
