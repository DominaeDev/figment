#include <pch.h>
#include "chat/ChatLogger.h"
#include "app/AppState.h"
#include "user/UserManager.h"
#include "io/AssetManager.h"
#include "io/Xml.h"
#include "chat/ChatSession.h"
#include "chat/MessagePoller.h"
#include "chat/ChatFormatter.h"
#include "util/StringUtils.h"

using namespace fig::io;
using namespace fig::data;

namespace fig::chat
{
	ChatLogger::ChatLogger(fig::uuid assetId, fig::uuid parentId, ChatSession& session) :
		_assetId { assetId },
		_parentId { parentId },
		_pSession { &session }
	{
		// Hook up to message poller
		if (auto poller = session.GetPoller())
			_pollerId = (*poller).RegisterObserver(std::bind_front(&ChatLogger::OnMessage, this), std::bind_front(&ChatLogger::OnRemovedMessage, this));
		else
			_pollerId = static_cast<uint32_t>(-1);

		if (not Load())
		{
			// Init log
			_log.SetTitle(session.GetStaging().GetStoryTitle());

			if constexpr (Disabled) //! Participants could potentially change
			{
				for (int32_t i = 0; i < MaxBots(); ++i)
				{
					Role role = bot_from_index(i);
					if (session.GetStaging().HasCharacterInRole(role))
						_log.AddSearchTerm(session.GetNameOf(role));
				}
				if (session.GetStaging().HasCharacterInRole(Role::User))
					_log.AddSearchTerm(session.GetNameOf(Role::User));
			}
		}
	}

	ChatLogger::~ChatLogger()
	{
		if (auto poller = _pSession->GetPoller())
			(*poller).UnregisterObserver(_pollerId);
	}

	void ChatLogger::OnMessage(const MessagePoller::Message& piece)
	{
		if (piece.complete)
		{
			_log.AddMessage(ChatMessage {
				.responseId = piece.responseId,
				.subMessageId = piece.subMessageId,
				.speakerId = _pSession->GetCharacterIdOf(piece.role),
				.role = piece.role,
				.turn = piece.turn,
				.subTurn = piece.subTurn,
				.timestamp = now(),
				.msgType = piece.msgType,
				.content = fig::string { ChatFormatter::Unwrap(piece.content) },
			});
		}
	}

	void ChatLogger::OnRemovedMessage(const MessagePoller::RemovedMessage& removed)
	{
		_log.RemoveMessage(removed.subMessageId);
	}

	bool ChatLogger::Load()
	{
		if (not Global::IsSignedIn())
		{
			assert(false && "Not signed in");
			return false;
		}

		return Global::GetUserContent().LoadXmlAsset(_assetId, _log);
	}

	bool ChatLogger::Save() const
	{
		if (not Global::IsSignedIn())
		{
			assert(false && "Not signed in");
			return false;
		}
		if (_assetId.empty())
			return false;

		// Write
		return Global::GetUserContent().UpdateAsset(_assetId, _log);
	}

	const std::vector<fig::data::ChatMessage>& ChatLogger::GetLogMessages() const noexcept
	{
		return _log.GetMessages();
	}

}

