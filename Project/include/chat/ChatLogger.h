#ifndef CHAT_LOGGER_H__
#define CHAT_LOGGER_H__
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
		ChatLogger(std::weak_ptr<ChatSession> pSession, fig::uuid assetId = {});

		bool Save();

	private:
		void OnMessage(const MessagePoller::Message& piece);
		
		fig::uuid _assetId;
		std::weak_ptr<ChatSession> _pSession {};
		fig::data::ChatLog _log {};

	};
}

#endif