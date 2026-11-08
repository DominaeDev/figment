#include <pch.h>
#include "gui/StoryCardList.h"
#include "gui/CardList.impl.h"
#include "gui/ScenarioCard.h"
#include "gui/Menu.h"

using namespace fig::io;
using namespace fig::user;

namespace fig::gui
{
	StoryCardList::StoryCardList(ControlPtr pParent, CardSize cardSize) : CardList<fig::data::Scenario>(pParent, cardSize)
	{
	}

	std::tuple<SortBy, OrderBy, FilterFlags> StoryCardList::GetSortOrder()
	{
		auto sortBy = Global::GetUserSettings().GetEnum<SortBy>(UserSetting::Interface::ScenarioList::Sorting, SortBy::Default);
		auto orderBy = Global::GetUserSettings().GetEnum<OrderBy>(UserSetting::Interface::ScenarioList::Ordering, OrderBy::Default);
		auto filterBy = Global::GetUserSettings().GetFlags<FilterFlags>(UserSetting::Interface::ScenarioList::Filtering, DefaultScenarioFilterFlags, FilterFlagMapping);
		return std::make_tuple(sortBy, orderBy, filterBy);
	}

	uint32_t StoryCardList::OnCardMenu(CardPtr pCard)
	{
		auto pScenarioCard = dynamic_cast<ScenarioCard*>(pCard.get());
		auto& scenarioId = pScenarioCard->GetAssetId();
		auto chatCount = pScenarioCard->GetChatCount();
		auto& userSettings = pScenarioCard->GetUserSettings();

		auto ChangeBorder = [&](CardBorderStyle border) {
			userSettings.borderStyle = border;
			Global::GetUserContent().SetBorder(scenarioId, border);
			pCard->SetBorder(border);
		};

		bool bLLM = Global::IsLLMInitialized();

		auto& menu = CreateMenu();

		menu.AddItem("Start a new chat", Icon::NEW_CHAT)
			.SetEnabled(bLLM);
			
		menu.AddSeparator();
		menu.AddItem("Edit story\u2026")
			.SetDelegate([&] {
				PushEvent(UserEvent::EditStory, &scenarioId);
			});

		auto& moveMenu = menu.AddItem("Move to folder");
		moveMenu.AddItem("New folder\u2026");
		menu.AddItem("Duplicate");
		menu.AddItem("Export\u2026");

		menu.AddSeparator();

		if (!userSettings.HasFlag(AssetUserSettings::Flag::Favorite))
		{
			menu.AddItem("Star", Icon::STAR)
				.SetDelegate([&] {
					Global::GetUserContent().MarkFavorite(scenarioId, true);
					userSettings.flags.Set(AssetUserSettings::Flag::Favorite);
					pCard->ShowStar(true);
					OnModifiedCard(pCard);
				});
		}
		else
		{
			menu.AddItem("Unstar", Icon::UNSTAR)
				.SetDelegate([&] {
					Global::GetUserContent().MarkFavorite(scenarioId, false);
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
					Global::GetUserContent().MarkHidden(scenarioId, true);
					userSettings.flags.Set(AssetUserSettings::Flag::Hidden);
					OnModifiedCard(pCard);
				});
		}
		else
		{
			menu.AddItem("Unhide")
				.SetDelegate([&] {
					Global::GetUserContent().MarkHidden(scenarioId, false);
					userSettings.flags.Unset(AssetUserSettings::Flag::Hidden);
					OnModifiedCard(pCard);
				});
		}

		menu.AddSeparator();

		menu.AddItem("Delete\u2026", Icon::DELETE)
			.SetDelegate([&] { 
				ShowDialogBox(std::format("Are you sure you want to delete {}?\nThis action cannot be undone.", pScenarioCard->GetName()), DialogBoxType::ConfirmCancel,
					[&](bool bOk) {
					if (bOk)
						OnDeleteCard(pScenarioCard);
				});
			});

		return menu.Show();
	}

	template class CardList<fig::data::Scenario>;
}