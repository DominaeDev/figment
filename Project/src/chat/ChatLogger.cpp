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
		_assetId { assetId }
	{
		if (auto poller = session.GetPoller())
			(*poller).RegisterObserver(std::bind_front(&ChatLogger::OnMessage, this));
	}

	void ChatLogger::OnMessage(const MessagePoller::Message& piece)
	{
		if (piece.complete)
		{
			_log.messages.push_back(ChatLog::Message {
				.messageId = piece.subMessageId,
				.timestamp = local_now(),
				.msgType = piece.msgType,
				.content = piece.content,
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

