#include <pch.h>
#include "gui/WorldCardList.h"
#include "gui/CardList.impl.h"
#include "gui/WorldCard.h"
#include "gui/Menu.h"

using namespace fig::io;
using namespace fig::user;

namespace fig::gui
{
	WorldCardList::WorldCardList(ControlPtr pParent, CardSize cardSize) : CardList<fig::data::World>(pParent, cardSize)
	{
	}

	std::tuple<SortBy, OrderBy, FilterFlags> WorldCardList::GetSortOrder()
	{
		auto sortBy = Global::GetUserSettings().GetEnum<SortBy>(UserSetting::Interface::WorldList::Sorting, SortBy::Default);
		auto orderBy = Global::GetUserSettings().GetEnum<OrderBy>(UserSetting::Interface::WorldList::Ordering, OrderBy::Default);
		auto filterBy = Global::GetUserSettings().GetFlags<FilterFlags>(UserSetting::Interface::WorldList::Filtering, DefaultWorldFilterFlags, FilterFlagMapping);
		return std::make_tuple(sortBy, orderBy, filterBy);
	}

	uint32_t WorldCardList::OnCardMenu(CardPtr pCard)
	{
		auto pWorldCard = dynamic_cast<WorldCard*>(pCard.get());
		auto& worldId = pWorldCard->GetAssetId();
		auto chatCount = pWorldCard->GetChatCount();
		auto& userSettings = pWorldCard->GetUserSettings();

		auto ChangeBorder = [&](CardBorderStyle border) {
			userSettings.borderStyle = border;
			Global::GetUserContent().SetBorder(worldId, border);
			pCard->SetBorder(border);
		};

		bool bLLM = Global::IsLLMInitialized();

		auto& menu = CreateMenu();

		menu.AddCheckItem("Set as default world", true);
			
		menu.AddSeparator();
		menu.AddItem("Edit world\u2026")
			.SetDelegate([&] {
				PushEvent(UserEvent::EditStory, &worldId);
			});

		CreateMoveMenu(menu, AssetFolderType::World, worldId, "Worlds");

		menu.AddItem("Duplicate");
		menu.AddItem("Export\u2026");

		menu.AddSeparator();

		if (!userSettings.HasFlag(AssetUserSettings::Flag::Favorite))
		{
			menu.AddItem("Star", Icon::STAR)
				.SetDelegate([&] {
					Global::GetUserContent().MarkFavorite(worldId, true);
					userSettings.flags.Set(AssetUserSettings::Flag::Favorite);
					pCard->ShowStar(true);
					OnModifiedCard(pCard);
				});
		}
		else
		{
			menu.AddItem("Unstar", Icon::UNSTAR)
				.SetDelegate([&] {
					Global::GetUserContent().MarkFavorite(worldId, false);
					userSettings.flags.Unset(AssetUserSettings::Flag::Favorite);
					pCard->ShowStar(false);
					OnModifiedCard(pCard);
				});
		}

		auto& borderMenu = menu.AddItem("Set border");
		borderMenu.AddCheckItem("No border", userSettings.borderStyle == CardBorderStyle::None)
			.SetDelegate([ChangeBorder] { ChangeBorder(CardBorderStyle::None); });
		borderMenu.AddSeparator();
		borderMenu.AddCheckItem("Border #1", userSettings.borderStyle == CardBorderStyle::Style01)
			.SetIcon(Icon::BORDER_01, false)
			.SetDelegate([ChangeBorder] { ChangeBorder(CardBorderStyle::Style01); });
		borderMenu.AddCheckItem("Border #2", userSettings.borderStyle == CardBorderStyle::Style02)
			.SetIcon(Icon::BORDER_02, false)
			.SetDelegate([ChangeBorder] { ChangeBorder(CardBorderStyle::Style02); });
		borderMenu.AddCheckItem("Border #3", userSettings.borderStyle == CardBorderStyle::Style03)
			.SetIcon(Icon::BORDER_03, false)
			.SetDelegate([ChangeBorder] { ChangeBorder(CardBorderStyle::Style03); });
		borderMenu.AddCheckItem("Border #4", userSettings.borderStyle == CardBorderStyle::Style04)
			.SetIcon(Icon::BORDER_04, false)
			.SetDelegate([ChangeBorder] { ChangeBorder(CardBorderStyle::Style04); });
		borderMenu.AddCheckItem("Border #5", userSettings.borderStyle == CardBorderStyle::Style05)
			.SetIcon(Icon::BORDER_05, false)
			.SetDelegate([ChangeBorder] { ChangeBorder(CardBorderStyle::Style05); });
		borderMenu.AddCheckItem("Border #6", userSettings.borderStyle == CardBorderStyle::Style06)
			.SetIcon(Icon::BORDER_06, false)
			.SetDelegate([ChangeBorder] { ChangeBorder(CardBorderStyle::Style06); });

		if (!userSettings.HasFlag(AssetUserSettings::Flag::Hidden))
		{
			menu.AddItem("Hide")
				.SetDelegate([&] {
					Global::GetUserContent().MarkHidden(worldId, true);
					userSettings.flags.Set(AssetUserSettings::Flag::Hidden);
					OnModifiedCard(pCard);
				});
		}
		else
		{
			menu.AddItem("Unhide")
				.SetDelegate([&] {
					Global::GetUserContent().MarkHidden(worldId, false);
					userSettings.flags.Unset(AssetUserSettings::Flag::Hidden);
					OnModifiedCard(pCard);
				});
		}

		menu.AddSeparator();

		menu.AddItem("Delete\u2026", Icon::DELETE)
			.SetDelegate([&] { 
				ShowDialogBox(std::format("Are you sure you want to delete {}?\nThis action cannot be undone.", pWorldCard->GetName()), DialogBoxType::ConfirmCancel,
					[&](bool bOk) {
					if (bOk)
						OnDeleteCard(pWorldCard);
				});
			});

		return menu.Show();
	}

	template class CardList<fig::data::World>;
}