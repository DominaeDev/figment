#include <pch.h>
#include "app/AppState.h"
#include "user/UserManager.h"
#include "io/AssetManager.h"
#include "io/Xml.h"
#include "chat/ChatLogger.h"
#include "chat/MessagePoller.h"

using namespace fig::io;
using namespace fig::data;

namespace fig::chat
{
	ChatLogger::ChatLogger(std::weak_ptr<ChatSession> pSession, fig::uuid assetId) :
		_pSession { pSession },
		_assetId { assetId }
	{
		
	}

	void ChatLogger::OnMessage(const MessagePoller::Message& piece)
	{
	}

	bool ChatLogger::Save()
	{
		if (not Global::GetUserManager().IsSignedIn())
			return false;
		


		if (_assetId.empty())
		{
			// Create new asset
			fig::bytes xmlData;
			_log.SaveToXml(xmlData);

//			auto& asset = Global::GetUserContent().GetAssetManager().CreateAsset(AssetType::ChatLog, DataFormat::DataXml, xmlData, _pSession->
		}
		return false;
	}

}

