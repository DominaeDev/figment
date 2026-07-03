#ifndef CHAT_INSTANCE_H__
#define CHAT_INSTANCE_H__
#pragma once

#include "Figment.h"
#include "chat/ChatOptions.h"
#include "io/IXmlSerializable.h"

namespace fig::data
{
	struct ChatInstance : public IXmlSerializable<"ChatInstance", 0>
	{
		fig::uuid scaffoldId;
		fig::uuid scenarioId;
		std::vector<fig::uuid> characterIds;
		fig::uuid userId;
		fig::chat::ChatOptions options;

		static auto SerializeInfo()
		{
			return Fields(
				AsElement { "Scaffold",		&ChatInstance::scaffoldId },
				AsElement { "Scenario",		&ChatInstance::scenarioId },
				AsElement { "Character",	&ChatInstance::characterIds },
				AsElement { "User",			&ChatInstance::userId },
				AsElement { "Options",		&ChatInstance::options }
			);
		}
	};
}

#endif