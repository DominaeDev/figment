#pragma once

#include "gui/Panel.h"
#include "util/SearchIndex.h"
#include "data/ContentMetaData.h"

namespace fig::gui
{
	enum class UserPersonaListItemEvent
	{
		Refresh,
		Hidden,
		Delete,
	};

	using UserPersonaListItemDelegate = std::function<void(class UserPersonaListItem&, UserPersonaListItemEvent event)>;

	class UserPersonaListItem : public Panel
	{
	public:
		UserPersonaListItem(ControlPtr pParent, const fig::uuid& userPersonaId);

		void SetDelegate(UserPersonaListItemDelegate fnDelegate) { _fnDelegate = fnDelegate; }

		const fig::uuid& GetAssetId() const { return _assetId; }
		const fig::io::ContentMetaData& GetMetaData() const noexcept { return _metaData; };
		fig::timestamp GetUpdatedAt() const noexcept { return _updatedAt; }

		bool MatchesFlags(fig::io::FilterFlags filter) const noexcept;
		bool MatchesSearch(const SearchQuery& query) const noexcept;

		void RefreshFull();
		void SetHidden(bool bHidden) noexcept;
		bool IsHidden() const noexcept { return _bHidden; }
	
		static constexpr fig::coord Width = 420;
		static constexpr fig::coord Height = 166;

	protected:
		void OnUpdate(float fElapsed) override;
		EventResult OnEvent(fig::event& event) override;

		void ShowMenu() noexcept;
		void NotifyUpdated();
		void NotifyHidden();
		void NotifyDelete();

		void ShowDefault(bool bShow);
		void SetIndex(const SearchIndex& index) noexcept;
		void RefreshMeta();
		void RefreshState();

		void ResetHover();

	private:
		fig::uuid _assetId;
		fig::io::ContentMetaData _metaData {};
		fig::io::AssetUserSettings _userSettings {};
		fig::timestamp _updatedAt {};
		bool _bHidden {};
		std::unique_ptr<SearchIndex> _searchIndex;

		fig::observer_ptr<ImageWithMask> _pPortrait;
		fig::observer_ptr<StaticText> _pTitle;
		fig::observer_ptr<StaticText> _pMessage;
		fig::observer_ptr<NineGridImage> _pDefaultIndicator;

		bool _bSelected {};
		bool _bHovered {};
		int32_t _menuId { -1 };
		UserPersonaListItemDelegate _fnDelegate {};

	};
}