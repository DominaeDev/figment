#pragma once

#include "Figment.h"
#include "chat/ChatOptions.h"
#include "io/XmlData.h"

namespace fig::data
{
	struct ChatInstance : public XmlData<"ChatInstance", 0>
	{
		std::vector<fig::uuid> characterIds;
		fig::uuid userId;
		fig::uuid scenarioId;
		fig::uuid worldId;
		fig::chat::ChatOptions options;

		static auto XmlFields()
		{
			return Fields(
				Element { "Character",		&ChatInstance::characterIds },
				Element { "User",			&ChatInstance::userId },
				Element { "Scenario",		&ChatInstance::scenarioId },
				Element { "World",			&ChatInstance::worldId },
				Element { "Options",		&ChatInstance::options }
			);
		}

		bool contains(fig::uuid id) const
		{
			return std::ranges::find(characterIds, id) != std::cend(characterIds)
				or userId == id
				or scenarioId == id
				or worldId == id;
		}
	};
}
