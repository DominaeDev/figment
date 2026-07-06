#include <pch.h>
#include "app/AppState.h"
#include "user/UserManager.h"
#include "io/AssetManager.h"
#include "io/Xml.h"
#include "chat/ChatSession.h"
#include "chat/ChatLogger.h"
#include "chat/MessagePoller.h"

using namespace fig::io;
using namespace fig::data;

namespace fig::chat
{
	ChatLogger::ChatLogger(ChatSession& session, fig::uuid parentID, fig::uuid assetId) :
		_parentId { parentID },
		_assetId { assetId },
		_pSession { &session }
	{
		if (auto poller = session.GetPoller())
			_pollerId = (*poller).RegisterObserver(std::bind_front(&ChatLogger::OnMessage, this));
		else
			_pollerId = static_cast<uint32_t>(-1);
	}

	ChatLogger::~ChatLogger()
	{
		if (auto poller = _pSession->GetPoller())
			(*poller).UnregisterObserver(_pollerId);
	}

	static fig::string strip_ends(const fig::string& text, MessageType msgType)
	{
		fig::string begin, end;
		switch (msgType)
		{
			case MessageType::Dialogue:
				begin = "\"";
				end = "\"";
				break;
			case MessageType::Action:
				begin = "*";
				end = "*";
				break;
			case MessageType::Direction:
				begin = "{";
				end = "}";
				break;
			case MessageType::Narration:
				begin = "[";
				end = "]";
				break;
			case MessageType::Thought:
				begin = "(";
				end = ")";
				break;
			default:
				return text;
		}

		fig::string stripped = text;
		while (ends_with(stripped, end))
			stripped = stripped.substr(0, stripped.length() - 1);
		while (begins_with(stripped, begin))
			stripped = stripped.substr(1);
		return stripped;
	}

	void ChatLogger::OnMessage(const MessagePoller::Message& piece)
	{
		if (piece.complete)
		{
			_log.messages.emplace_back(ChatLog::Message {
				.messageId = piece.subMessageId,
				.speakerId = _pSession->GetCharacterIdOf(piece.role),
				.role = piece.role,
				.turn = piece.turn,
				.subTurn = piece.subTurn,
				.timestamp = local_now(),
				.msgType = piece.msgType,
				.content = strip_ends(piece.content, piece.msgType),
			});
		}
	}

	bool ChatLogger::Save()
	{
		if (not Global::GetUserManager().IsSignedIn())
		{
			assert(false && "Not logged in");
			return false;
		}

		auto& assetMngr = Global::GetUserContent().GetAssetManager();

		if (_assetId.empty())
		{
			// Create new asset
			fig::bytes xmlData;
			_log.SaveToXml(xmlData);

			fig::string tmp;
			tmp.assign(reinterpret_cast<const char*>(xmlData.data()), xmlData.size()); //! @temp

			auto& asset = assetMngr.CreateAsset(AssetType::ChatLog, DataFormat::DataXml, xmlData, _parentId);
			_assetId = asset.id;
			return true;
		}
		else
		{
			// Update asset
			if (auto try_asset = assetMngr.FindAsset(_assetId, AssetType::ChatLog))
			{
				assetMngr.ModifyAsset(*try_asset, [&](Asset& asset) {
					fig::bytes xmlData;
					_log.SaveToXml(xmlData);
					asset.SetData(xmlData);
				});
				return true;
			}
		}

		// Error
		assert(false && "Invalid asset id");
		return false;
	}

}

