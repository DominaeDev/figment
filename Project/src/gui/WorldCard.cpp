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
				.name = _name,
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
			_name = world.name;
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
}