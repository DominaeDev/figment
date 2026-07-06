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
			bool complete {};
			int32_t turn {};
			int32_t subTurn {};
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

		std::vector<Message> _messages {};
		std::map<fig::uuid, size_t> _messagesById {}; // Sub-message id

		std::unordered_map<CallbackId, Callback> _observers {};
		CallbackId _nextCallbackId = 0;
	};
}
