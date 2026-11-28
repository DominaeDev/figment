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
		std::vector<size_t> removedMessages;

		std::vector<MessagePiece> pieces;
		std::vector<fig::llm::RemovedMessage> removedMsgs;
		if (pLLM->PollResponse(pieces, removedMsgs))
		{
			for (auto& removedMsg : removedMsgs)
			{
				for (size_t i = 0; i < _messages.size(); i++)
				{
					if (_messages[i].responseId == removedMsg.responseId)
						removedMessages.push_back(i);
				}
			}

			for (auto& piece : pieces)
			{
				auto findMsg = _messagesById.find(piece.subMessageId);
				if (findMsg != _messagesById.end())
				{
					// Append piece
					auto& entry = _messages[findMsg->second];

					entry.role = piece.role;
					entry.msgType = piece.msgType;
					entry.complete = piece.isComplete;
					entry.allowTTS = piece.allowTTS;
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
						.allowTTS = piece.allowTTS,
						.turn = piece.turn,
						.subTurn = piece.subMessageIndex,
					});

					_messagesById[piece.subMessageId] = _messages.size() - 1uz;
					modifiedMessages.insert(_messages.size() - 1uz);
				}
			}
		}

		if (modifiedMessages.empty() and removedMessages.empty())
			return;

		std::ranges::sort(removedMessages, std::ranges::greater());

		// Notify observers
		for (auto& observer : _observers)
		{
			if (auto& fnDelegate = observer.second.fnOnMessage)
			{
				for (auto index : modifiedMessages)
				{
					const auto& message = _messages[index];
					fnDelegate(message);
				}
			}

			if (auto& fnDelegate = observer.second.fnOnRemovedMessage)
			{
				for (auto index : removedMessages)
				{
					const auto& message = _messages[index];
					fnDelegate(RemovedMessage {
						.responseId = message.responseId,
						.subMessageId = message.subMessageId,
					});
				}
			}
		}

		for (auto index : removedMessages)
		{
			auto& message = _messages[index];
			_messagesById.erase(message.subMessageId);
			erase_at(_messages, index);
		}
	}

	MessagePoller::CallbackId MessagePoller::RegisterObserver(MessageCallback onMessage, RemovedMessageCallback onRemovedMessage)
	{
		_observers.emplace(_nextCallbackId, Observer {
			std::move(onMessage),
			std::move(onRemovedMessage),
		});
		return _nextCallbackId++;
	}

	void MessagePoller::UnregisterObserver(CallbackId id)
	{
		_observers.erase(id);
	}

}