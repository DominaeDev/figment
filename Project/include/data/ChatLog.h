#ifndef CHAT_LOG_H__
#define CHAT_LOG_H__
#pragma once

#include "Figment.h"
#include "chat/ChatTypes.h"

namespace fig::data
{
	class ChatLog
	{
	public:
		struct Entry
		{
			fig::uuid entryId;
			fig::uuid speakerId;
			size_t turn;
			fig::chat::Role role;
			fig::chat::MessageType msgType;
			uint16_t subindex;
			uint32_t _reserved;
			fig::timestamp timestamp;
			fig::string content;

			static auto SerializeInfo() noexcept;
		};

		fig::uuid assetId;
		std::vector<Entry> entries;

		static auto SerializeInfo() noexcept;
	};
}


#endif