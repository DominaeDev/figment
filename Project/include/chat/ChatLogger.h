#pragma once

#include "Figment.h"
#include "data/ChatLog.h"
#include "chat/MessagePoller.h"

namespace fig::chat
{
	class ChatSession;

	class ChatLogger
	{
	public:
		ChatLogger(ChatSession& session, fig::uuid parentID, fig::uuid assetId = {});
		~ChatLogger();

		bool Save();

	private:
		void OnMessage(const MessagePoller::Message& piece);
		
		fig::uuid _assetId;
		fig::uuid _parentId;
		fig::data::ChatLog _log {};
		fig::observer_ptr<ChatSession> _pSession;
		uint32_t _pollerId {};
	};
}
