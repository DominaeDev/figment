#include <pch.h>
#include "gui/ScenarioCard.h"
#include "gui/AppResources.h"
#include "app/AppState.h"
#include "user/UserManager.h"
#include "gui/Events.h"
#include "gui/Menu.h"

using namespace fig::data;
using namespace fig::io;

namespace fig::gui
{
	ScenarioCard::ScenarioCard(ControlPtr pParent, const fig::uuid& scenarioId, CardSize cardSize) : CoverCard(pParent, scenarioId, cardSize),
		_scenarioId { scenarioId }
	{
		SetDefaultImage(AppResources::GetTexture(Resource::CARD_COVER_STORY));
		RefreshFull();
	}

	EventResult ScenarioCard::OnEvent(fig::event& event)
	{
		switch (event.type)
		{
		case SDL_EVENT_MOUSE_BUTTON_UP:
			if (event.button.button == SDL_BUTTON_RIGHT
				and is_inside(GetRect(), toI(event.button.x), toI(event.button.y))
				and not _bHidden)
			{
				ShowMenu();
				return EventResult::Handled;
			}
			break;
		}

		if (IsUserEvent(event, UserEvent::MenuOpened))
		{
			if (_menuId == event.user.code)
			{
				_bSelected = true;
				return EventResult::Continue;
			}
		}
		else if (IsUserEvent(event, UserEvent::MenuClosed))
		{
			if (_menuId == event.user.code)
			{
				_bSelected = false;
				return EventResult::Continue;
			}
		}
		else if (IsUserEvent(event, UserEvent::ScreenDeactivated))
		{
			_bSelected = false;
			ResetHoverZoom();
			return EventResult::Continue;
		}

		return EventResult::Pass;
	}

	void ScenarioCard::ShowMenu()
	{
		auto ChangeBorder = [this](CardBorderStyle border) {
			_userSettings.borderStyle = border;
			Global::GetUserContent().SetBorder(_scenarioId, border);
			SetBorder(border);
			};

		bool bLLM = Global::IsLLMInitialized();

		auto& menu = CreateMenu();

		menu.AddItem("Start a new chat", Icon::NEW_CHAT)
			.SetEnabled(bLLM);
			
		menu.AddSeparator();
		menu.AddItem("Edit story\u2026")
			.SetDelegate([this] {
				PushEvent(UserEvent::EditStory, &_scenarioId);
			});

		auto& moveMenu = menu.AddItem("Move to folder");
		moveMenu.AddItem("New folder\u2026");
		menu.AddItem("Duplicate");
		menu.AddItem("Export\u2026");

		menu.AddSeparator();

		if (!_userSettings.HasFlag(AssetUserSettings::Flag::Favorite))
		{
			menu.AddItem("Star", Icon::STAR)
				.SetDelegate([this] {
					Global::GetUserContent().MarkFavorite(_scenarioId, true);
					_userSettings.flags.Set(AssetUserSettings::Flag::Favorite);
					ShowStar(true);
					NotifyUpdated();
				});
		}
		else
		{
			menu.AddItem("Unstar", Icon::UNSTAR)
				.SetDelegate([this] {
					Global::GetUserContent().MarkFavorite(_scenarioId, false);
					_userSettings.flags.Unset(AssetUserSettings::Flag::Favorite);
					ShowStar(false);
					NotifyUpdated();
				});
		}

		auto& borderMenu = menu.AddItem("Set border");
		borderMenu.AddCheckItem("No border", _userSettings.borderStyle == CardBorderStyle::None)
			.SetDelegate([ChangeBorder] { ChangeBorder(CardBorderStyle::None); });
		borderMenu.AddSeparator();
		borderMenu.AddCheckItem("Border #1", _userSettings.borderStyle == CardBorderStyle::Style01)
			.SetIcon(Icon::BORDER_01, false)
			.SetDelegate([ChangeBorder] { ChangeBorder(CardBorderStyle::Style01); });
		borderMenu.AddCheckItem("Border #2", _userSettings.borderStyle == CardBorderStyle::Style02)
			.SetIcon(Icon::BORDER_02, false)
			.SetDelegate([ChangeBorder] { ChangeBorder(CardBorderStyle::Style02); });
		borderMenu.AddCheckItem("Border #3", _userSettings.borderStyle == CardBorderStyle::Style03)
			.SetIcon(Icon::BORDER_03, false)
			.SetDelegate([ChangeBorder] { ChangeBorder(CardBorderStyle::Style03); });
		borderMenu.AddCheckItem("Border #4", _userSettings.borderStyle == CardBorderStyle::Style04)
			.SetIcon(Icon::BORDER_04, false)
			.SetDelegate([ChangeBorder] { ChangeBorder(CardBorderStyle::Style04); });
		borderMenu.AddCheckItem("Border #5", _userSettings.borderStyle == CardBorderStyle::Style05)
			.SetIcon(Icon::BORDER_05, false)
			.SetDelegate([ChangeBorder] { ChangeBorder(CardBorderStyle::Style05); });
		borderMenu.AddCheckItem("Border #6", _userSettings.borderStyle == CardBorderStyle::Style06)
			.SetIcon(Icon::BORDER_06, false)
			.SetDelegate([ChangeBorder] { ChangeBorder(CardBorderStyle::Style06); });

		if (!_userSettings.HasFlag(AssetUserSettings::Flag::Hidden))
		{
			menu.AddItem("Hide")
				.SetDelegate([this] {
					Global::GetUserContent().MarkHidden(_scenarioId, true);
					_userSettings.flags.Set(AssetUserSettings::Flag::Hidden);
					NotifyUpdated();
				});
		}
		else
		{
			menu.AddItem("Unhide")
				.SetDelegate([this] {
					Global::GetUserContent().MarkHidden(_scenarioId, false);
					_userSettings.flags.Unset(AssetUserSettings::Flag::Hidden);
					NotifyUpdated();
				});
		}

		menu.AddSeparator();

		menu.AddItem("Delete\u2026", Icon::DELETE)
			.SetDelegate([this] { NotifyDelete(); });

		_menuId = menu.Show();
	}

	void ScenarioCard::RefreshMeta()
	{
		auto& userContent = Global::GetUserContent();
		// Meta data
		if (auto try_meta = userContent.GetMetaData(_scenarioId))
		{
			auto& meta = *try_meta;
			SetMetaData(meta);
		}
		else // Error
		{
			auto now = fig::now();
			SetMetaData(ContentMetaData {
				.name = _scenarioName,
				.createdAt = now,
				.updatedAt = now,
				.lastUsedAt = now,
			});
		}

		// User settings
		SetUserSettings(Global::GetUserContent().GetUserSettings(_scenarioId));
	}

	void ScenarioCard::RefreshFull()
	{
		if (auto try_scenario = Global::GetUserContent().Get<Scenario>(_scenarioId); try_scenario.has_value())
		{
			auto& scenario = try_scenario.value();
			_scenarioName = scenario.name;
			if (_scenarioName.empty())
				_scenarioName = scenario.title;
			_updatedAt = Global::GetUserContent().GetUpdatedAt(_scenarioId).value_or({});
			SetLabel(scenario.name);
//			SetIndex(scenario.GetSearchIndex());

			// Tags
			ClearTags();
			auto& tags = scenario.GetTags();
			for (size_t i = 0; i < tags.size(); ++i)
			{
				if (AddTag(tags[i]) == CoverCard::AddTagResult::Stop)
					break;
			}
		}

		RefreshMeta();
	}

	void ScenarioCard::NotifyDelete()
	{
		ShowDialogBox(std::format("Are you sure you want to delete {}?\nThis action cannot be undone.", _scenarioName), DialogBoxType::ConfirmCancel,
			[this](bool bOk) {
				if (_fnDelegate and bOk)
					_fnDelegate(*this, CardEvent::Delete);
		});
	}

}