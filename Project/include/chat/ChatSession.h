#pragma once

#include "chat/ChatStaging.h"
#include "chat/ChatLogger.h"

namespace fig::chat
{
	class MessagePoller;

	class ChatSession
	{
	public:
		ChatSession();
		ChatSession(const ChatSession&) = delete;
		ChatSession(ChatSession&&) = default;
		~ChatSession();

		void Initialize(const ChatStaging& staging, ChatOptions options, fig::uuid chatInstanceID);	
		void Save();
		void Shutdown();

		inline const ChatStaging& GetStaging() const noexcept { return _staging; }
		inline ChatStaging& GetStaging() noexcept { return _staging; }

		fig::uuid GetCharacterIdOf(Role role) const;
		fig::string GetIdentifierOf(Role role) const;
		fig::string GetNameOf(Role role) const;
		fig::string GetNameGrammar(bool useCharacterIds, bool bIncludeUser) const;
		fig::color_pair GetColorsOf(Role role) const;
		static fig::color_pair GetDefaultColorsOf(Role role);

		inline const Context& GetContext() const noexcept { return _context; }

		void Update(float fElapsed) noexcept;

		fig::optional_ref<MessagePoller> GetPoller() noexcept;

	protected:
		bool _bInitialized { false };
		ChatStaging _staging {};
		ChatOptions _options {};
		Context _context {};
		std::unique_ptr<MessagePoller> _messagePoller {};
		std::unique_ptr<ChatLogger> _logger {};
	};
}