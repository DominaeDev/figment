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

		bool Save();

	private:
		void OnMessage(const MessagePoller::Message& piece);
		
		fig::uuid _assetId;
		fig::uuid _parentId;
		fig::data::ChatLog _log {};

	};
}
