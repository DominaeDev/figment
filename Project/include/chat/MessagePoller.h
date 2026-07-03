#ifndef MESSAGE_POLLER_H__
#define MESSAGE_POLLER_H__
#pragma once

namespace fig::chat
{
	class MessagePoller
	{
	public:
		struct Message
		{
			fig::uuid responseId;
			fig::uuid subMessageId;
			fig::chat::Role role { fig::chat::Role::Undefined };
			fig::chat::MessageType msgType { fig::chat::MessageType::Undefined };
			fig::string content;
			bool complete = false;
		};
		using CallbackId = uint32_t;
		using Callback = std::function<void(const Message&)>;

		CallbackId RegisterObserver(Callback fnCallback);
		void UnregisterObserver(CallbackId id);

		void Update(float fElapsed);

	private:
		void Poll();

	private:
		float _fPollTimer = 0.0f;

		std::vector<std::shared_ptr<Message>> _messages {};
		std::map<fig::uuid, std::shared_ptr<Message>> _messagesById {}; // Sub-message id

		std::unordered_map<CallbackId, Callback> _observers {};
		CallbackId _nextCallbackId = 0;
	};
}
#endif