#include <pch.h>
#include "chat/MessagePoller.h"
#include "llm/LLMInstance.h"
#include "app/AppState.h"

using namespace fig::llm;
using namespace fig::data;

namespace fig::chat
{
	static constexpr float kPollInterval = 0.1f;

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

		std::unordered_set<size_t> modifiedMessages;

		MessagePiece piece;
		while (pLLM->PollResponse(piece))
		{
			auto findMsg = _messagesById.find(piece.subMessageId);
			if (findMsg != _messagesById.end())
			{
				// Append piece
				auto& entry = _messages[findMsg->second];

				entry.role = piece.role;
				entry.msgType = piece.msgType;
				entry.complete = piece.isComplete;
				entry.content += piece.content;
				entry.turn = piece.turn;
				entry.subTurn = piece.subMessageIndex;

				modifiedMessages.insert(findMsg->second);
			}
			else if (not (piece.isComplete and empty_or_whitespace(piece.content))) // Ignore (new) empty messages
			{
				// New message
				_messages.emplace_back(Message {
					.responseId = piece.responseId,
					.subMessageId = piece.subMessageId,
					.role = piece.role,
					.msgType = piece.msgType,
					.content = piece.content,
					.complete = piece.isComplete,
					.turn = piece.turn,
					.subTurn = piece.subMessageIndex,
				});

				_messagesById[piece.subMessageId] = _messages.size() - 1uz;
				modifiedMessages.insert(_messages.size() - 1uz);
			}
		}

		// Notify observers
		for (auto index : modifiedMessages)
		{
			const auto& message = _messages[index];
			for (auto& observer : _observers)
				observer.second(message);
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