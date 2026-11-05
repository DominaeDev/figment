#include <pch.h>
#include "gui/WorldCard.h"
#include "gui/AppResources.h"
#include "app/AppState.h"
#include "user/UserManager.h"
#include "gui/Events.h"
#include "gui/Menu.h"

using namespace fig::data;
using namespace fig::io;

namespace fig::gui
{
	WorldCard::WorldCard(ControlPtr pParent, const fig::uuid& worldId, CardSize cardSize) : CoverCard(pParent, worldId, cardSize)
	{
		SetDefaultImage(AppResources::GetTexture(Resource::CARD_COVER_WORLD));
		RefreshFull();
	}

	EventResult WorldCard::OnEvent(fig::event& event)
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

	void WorldCard::ShowMenu()
	{
		auto ChangeBorder = [this](CardBorderStyle border) {
			_userSettings.borderStyle = border;
			Global::GetUserContent().SetBorder(_assetId, border);
			SetBorder(border);
			};

		bool bLLM = Global::IsLLMInitialized();

		auto& menu = CreateMenu();

		menu.AddCheckItem("Set as default world", true);
		menu.AddSeparator();

		menu.AddItem("Edit world\u2026")
			.SetDelegate([this] {
				PushEvent(UserEvent::EditWorld, &_assetId);
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
					Global::GetUserContent().MarkFavorite(_assetId, true);
					_userSettings.flags.Set(AssetUserSettings::Flag::Favorite);
					ShowStar(true);
					NotifyUpdated();
				});
		}
		else
		{
			menu.AddItem("Unstar", Icon::UNSTAR)
				.SetDelegate([this] {
					Global::GetUserContent().MarkFavorite(_assetId, false);
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
					Global::GetUserContent().MarkHidden(_assetId, true);
					_userSettings.flags.Set(AssetUserSettings::Flag::Hidden);
					NotifyUpdated();
				});
		}
		else
		{
			menu.AddItem("Unhide")
				.SetDelegate([this] {
					Global::GetUserContent().MarkHidden(_assetId, false);
					_userSettings.flags.Unset(AssetUserSettings::Flag::Hidden);
					NotifyUpdated();
				});
		}

		menu.AddSeparator();

		menu.AddItem("Delete\u2026", Icon::DELETE)
			.SetDelegate([this] { NotifyDelete(); });

		_menuId = menu.Show();
	}

	void WorldCard::RefreshMeta()
	{
		auto& userContent = Global::GetUserContent();
		// Meta data
		if (auto try_meta = userContent.GetMetaData(_assetId))
		{
			auto& meta = *try_meta;
			SetMetaData(meta);
		}
		else // Error
		{
			auto now = fig::now();
			SetMetaData(ContentMetaData {
				.name = _worldName,
				.createdAt = now,
				.updatedAt = now,
				.lastUsedAt = now,
			});
		}

		// User settings
		SetUserSettings(Global::GetUserContent().GetUserSettings(_assetId));
	}

	void WorldCard::RefreshFull()
	{
		if (auto try_world = Global::GetUserContent().Get<World>(_assetId); try_world.has_value())
		{
			auto& world = try_world.value();
			_worldName = world.name;
			_updatedAt = Global::GetUserContent().GetUpdatedAt(_assetId).value_or({});
			SetLabel(world.name);
//			SetIndex(world.GetSearchIndex());

			// Tags
			ClearTags();
			auto& tags = world.GetTags();
			for (size_t i = 0; i < tags.size(); ++i)
			{
				if (AddTag(tags[i]) == CoverCard::AddTagResult::Stop)
					break;
			}
		}

		RefreshMeta();
	}

	void WorldCard::NotifyDelete()
	{
		ShowDialogBox(std::format("Are you sure you want to delete {}?\nThis action cannot be undone.", _worldName), DialogBoxType::ConfirmCancel,
			[this](bool bOk) {
				if (_fnDelegate and bOk)
					_fnDelegate(*this, CardEvent::Delete);
		});
	}

}