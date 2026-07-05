#include <pch.h>
#include "chat/MessagePoller.h"
#include "llm/LLMInstance.h"
#include "app/AppState.h"

using namespace fig::llm;
using namespace fig::data;

namespace fig::chat
{
	static constexpr float kPollInterval = 0.2f;

	void MessagePoller::Update(float fElapsed)
	{
		// Poll LLM for new messages
		_fPollTimer += fElapsed;
		if (_fPollTimer >= kPollInterval)
		{
			_fPollTimer = 0.0f;
			Poll();
		}
	}

	void MessagePoller::Poll()
	{
		auto pLLM = Global::GetLLMInstance();
		if (!pLLM)
			return;

		std::unordered_set<std::shared_ptr<Message>> modifiedMessages;

		MessagePiece piece;
		while (pLLM->PollResponse(piece))
		{
			auto findMsg = _messagesById.find(piece.subMessageId);
			if (findMsg != _messagesById.end())
			{
				// Append piece
				auto& entry = *(findMsg->second);

				entry.role = piece.role;
				entry.msgType = piece.msgType;
				entry.complete = piece.isComplete;
				entry.content += piece.content;

				modifiedMessages.insert(findMsg->second);
			}
			else if (not (piece.isComplete and empty_or_whitespace(piece.content))) // Ignore (new) empty messages
			{
				// New message
				_messages.push_back(std::make_shared<Message>(Message {
					.responseId = piece.responseId,
					.subMessageId = piece.subMessageId,
					.role = piece.role,
					.msgType = piece.msgType,
					.content = piece.content,
				}));

				_messagesById[piece.subMessageId] = _messages.back();
				modifiedMessages.insert(_messages.back());
			}
		}

		// Notify observers
		for (auto& message : modifiedMessages)
		{
			for (auto& observer : _observers)
				observer.second(*message);
		}
	}

	MessagePoller::CallbackId MessagePoller::RegisterObserver(Callback fnCallback)
	{
		_observers.emplace(_nextCallbackId, std::move(fnCallback));
		return _nextCallbackId++;
	}

	void MessagePoller::UnregisterObserver(CallbackId id)
	{
		_observers.erase(id);
	}

}