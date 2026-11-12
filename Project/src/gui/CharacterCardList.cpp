#include <pch.h>
#include "gui/CharacterCardList.h"
#include "gui/CardList.impl.h"
#include "gui/CharacterCard.h"
#include "gui/Menu.h"

using namespace fig::io;
using namespace fig::user;

namespace fig::gui
{
	CharacterCardList::CharacterCardList(ControlPtr pParent, CardSize cardSize) : CardList<fig::data::Character>(pParent, cardSize)
	{
	}

	std::tuple<SortBy, OrderBy, FilterFlags> CharacterCardList::GetSortOrder()
	{
		auto sortBy = Global::GetUserSettings().GetEnum<SortBy>(UserSetting::Interface::CharacterList::Sorting, SortBy::Default);
		auto orderBy = Global::GetUserSettings().GetEnum<OrderBy>(UserSetting::Interface::CharacterList::Ordering, OrderBy::Default);
		auto filterBy = Global::GetUserSettings().GetFlags<FilterFlags>(UserSetting::Interface::CharacterList::Filtering, DefaultCharacterFilterFlags, FilterFlagMapping);
		return std::make_tuple(sortBy, orderBy, filterBy);
	}

	uint32_t CharacterCardList::OnCardMenu(CardPtr pCard)
	{
		auto pCharacterCard = dynamic_cast<CharacterCard*>(pCard.get());
		auto& characterId = pCharacterCard->GetAssetId();
		auto chatCount = pCharacterCard->GetChatCount();
		const auto& userSettings = pCharacterCard->GetUserSettings();

		auto ChangeBorder = [this, characterId, pCharacterCard](CardBorderStyle border) {
			pCharacterCard->GetUserSettings().borderStyle = border;
			Global::GetUserContent().SetBorder(characterId, border);
			pCharacterCard->SetBorder(border);
		};

		bool bLLM = Global::IsLLMInitialized();

		auto& menu = CreateMenu();

		menu.AddItem("Resume chat")
			.SetEnabled(bLLM && chatCount > 0);
		menu.AddItem("Start a new chat", Icon::NEW_CHAT)
			.SetEnabled(bLLM)
			.SetDelegate([this, characterId] { 
				PushEvent(UserEvent::StartChat, &characterId); 
			});
		menu.AddItem("View chats")
			.SetEnabled(chatCount > 0)
			.SetDelegate([this, characterId] { 
				PushEvent(UserEvent::NavigateToChatList, &characterId); 
			});

		if constexpr (Debugging)
		{
			menu.AddItem(std::format("Debug {}\u2026", pCharacterCard->GetName()))
				.SetDelegate([this, characterId] {
					PushEvent(UserEvent::DebugCharacter, &characterId);
				});
		}
		
		menu.AddSeparator();
		menu.AddItem("Edit character\u2026", Icon::EDIT)
			.SetDelegate([this, characterId] {
				PushEvent(UserEvent::EditCharacter, &characterId);
			});
		menu.AddItem("Edit story\u2026")
			.SetDelegate([this, characterId] {
				PushEvent(UserEvent::EditCharacterStory, &characterId);
			});

		CreateMoveMenu(menu, AssetFolderType::Character, characterId, fig::strings::UI::MenuCharacters);

		menu.AddItem("Duplicate");
		menu.AddItem("Export\u2026");

		menu.AddSeparator();

		if (!userSettings.HasFlag(AssetUserSettings::Flag::Favorite))
		{
			menu.AddItem("Star", Icon::STAR)
				.SetDelegate([this, characterId, pCharacterCard] {
					Global::GetUserContent().MarkFavorite(characterId, true);
					pCharacterCard->GetUserSettings().flags.Set(AssetUserSettings::Flag::Favorite);
					pCharacterCard->ShowStar(true);
					OnModifiedCard(pCharacterCard);
				});
		}
		else
		{
			menu.AddItem("Unstar", Icon::UNSTAR)
				.SetDelegate([this, characterId, pCharacterCard] {
				Global::GetUserContent().MarkFavorite(characterId, false);
				pCharacterCard->GetUserSettings().flags.Unset(AssetUserSettings::Flag::Favorite);
				pCharacterCard->ShowStar(false);
				OnModifiedCard(pCharacterCard);
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
				.SetDelegate([this, characterId, pCharacterCard] { 
					Global::GetUserContent().MarkHidden(characterId, true);
					pCharacterCard->GetUserSettings().flags.Set(AssetUserSettings::Flag::Hidden);
					OnModifiedCard(pCharacterCard);
				});
		}
		else
		{
			menu.AddItem("Unhide")
				.SetDelegate([this, characterId, pCharacterCard] { 
					Global::GetUserContent().MarkHidden(characterId, false);
					pCharacterCard->GetUserSettings().flags.Unset(AssetUserSettings::Flag::Hidden);
					OnModifiedCard(pCharacterCard);
				});
		}
	
		menu.AddSeparator();
	
		menu.AddItem("Delete\u2026", Icon::DELETE)
			.SetDelegate([this, characterId, pCharacterCard] { 
				ShowDialogBox(std::format("Are you sure you want to delete {}?\nThis action cannot be undone.", pCharacterCard->GetName()), DialogBoxType::ConfirmCancel,
					[this, pCharacterCard](bool bOk) {
					if (bOk)
						OnDeleteCard(pCharacterCard);
				});
			});

		return menu.Show();
	}

	template class CardList<fig::data::Character>;
}