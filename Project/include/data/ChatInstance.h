#pragma once

#include "Figment.h"
#include "chat/ChatOptions.h"
#include "io/XmlData.h"

namespace fig::data
{
	struct ChatInstance : public XmlData<"ChatInstance", 0>
	{
		fig::uuid scenarioId;
		std::vector<fig::uuid> characterIds;
		fig::uuid userId;
		fig::chat::ChatOptions options;

		static auto XmlFields()
		{
			return Fields(
				Element { "Scenario",		&ChatInstance::scenarioId },
				Element { "Character",	&ChatInstance::characterIds },
				Element { "User",			&ChatInstance::userId },
				Element { "Options",		&ChatInstance::options }
			);
		}
	};
}
